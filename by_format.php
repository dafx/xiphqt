<?php

include_once(dirname(__FILE__).'/inc/prepend.php');

// Get the args
if (array_key_exists('PATH_INFO', $_SERVER))
{
	$search_string = preg_replace('|^/([^\s/]+).*$|', '$1', $_SERVER['PATH_INFO']);
	$search_string = substr($search_string, 0, 15);
	$tpl->assign('search_keyword', str_replace('_', ' ', $search_string));
	$search_string = preg_replace('/[^A-Za-z0-9+_\-]/', '_', $search_string);
	$search_string_hash = jenkins_hash_hex($search_string);
	
	// Memcache connection
    $memcache = DirXiphOrgMCC::getInstance();
	
	// Get the data from the Memcache server
	if (($results = $memcache->get('prod_search_format_'.$search_string_hash)) === false)
	{
		// Database connection
		$db = DirXiphOrgDBC::getInstance();
	
		// Cache miss. Now query the database.
		try
		{
			$query = ' SELECT m.* FROM `mountpoint` AS m INNER JOIN `media_type` AS mty ON m.`media_type_id` = mty.`id` WHERE mty.`media_type_url` = "%s" ORDER BY m.`listeners` DESC LIMIT %d;';
			$query = sprintf($query, mysql_real_escape_string($search_string), MAX_SEARCH_RESULTS);
			$results = $db->selectQuery($query);
			$res = array();
			while (!$results->endOf())
			{
				$res[] = Mountpoint::retrieveByPk($results->current('id'));
				$results->next();
			}
			$results = $res;
		}
		catch (SQLNoResultException $e)
		{
			$results = array();
		}
	
		// Cache the resultset
		$memcache->set('prod_search_format_'.$search_string_hash, $results, false, 60);
	}
	
	if ($results !== false && $results !== array())
	{
	    $n_results = count($results);
		$results_pages = $n_results / MAX_RESULTS_PER_PAGE;
		if ($page_n > $results_pages)
		{
		    $page_n = 0;
		}
		$offset = $page_n * MAX_RESULTS_PER_PAGE;
	    $results = array_slice($results, $offset,
	                                     MAX_RESULTS_PER_PAGE);
		$tpl->assign_by_ref('results', $results);
		$tpl->assign_by_ref('results_pages', $results_pages);
		$tpl->assign_by_ref('results_page_no', $page_n);
	}
}
else
{
	// Tag cloud
//	$tpl->assign('tag_cloud', $memcache->get('prod_tagcloud'));
}

// Header
$tpl->display("head.tpl");

// Display the results
$tpl->assign('servers_total', $memcache->get('servers_total'));
$tpl->assign('servers_mp3', $memcache->get('servers_'.CONTENT_TYPE_MP3));
$tpl->assign('servers_vorbis', $memcache->get('servers_'.CONTENT_TYPE_OGG_VORBIS));
$tpl->assign('tag_cloud', $memcache->get('prod_tagcloud'));
$tpl->display('search.tpl');

// Footer
$tpl->assign('generation_time', (microtime(true) - $begin_time) * 1000);
$tpl->assign('sql_queries', isset($db) ? $db->queries : 0);
if (isset($db))
{
	$tpl->assign('sql_debug', $db->log);	
}
$tpl->assign('mc_gets', isset($memcache) ? $memcache->gets : 0);
$tpl->assign('mc_sets', isset($memcache) ? $memcache->sets : 0);
if (isset($memcache))
{
    $tpl->assign('mc_debug', $memcache->log);
}
$tpl->display('foot.tpl');

?>
