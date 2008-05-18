<?php

define('FILE_TYPE_M3U', 120);
define('FILE_TYPE_XSPF', 121);

/******************************************************************************/
/*                                CONTROLLER                                  */
/******************************************************************************/

// Get request parameters
$p_id = 0;
$file_type = FILE_TYPE_M3U;
if (!empty($_GET))
{
    // PID
    if (!array_key_exists('pid', $_GET))
    {
	    header('HTTP/1.1 400 Bad Request', true);
	    die("Missing PID.\n");
    }
    if (!preg_match('/^(\d+)$/', $_GET['pid']))
    {
	    header('HTTP/1.1 400 Bad Request', true);
	    die("Bad PID.\n");
    }
    $p_id = intval($_GET['pid']);
    
    // Filetype
    if (array_key_exists('file', $_GET) && $_GET['file'] == 'listen.xspf')
    {
        $file_type = FILE_TYPE_XSPF;
    }
}
elseif (empty($_GET) && !empty($_SERVER['PATH_INFO']))
{
    // PID
    $data = explode('/', $_SERVER['PATH_INFO']);
    if (!array_key_exists(1, $data))
    {
	    header('HTTP/1.1 400 Bad Request', true);
	    die("Missing PID.\n");
    }
    if (!preg_match('/^(\d+)$/', $data[1]))
    {
	    header('HTTP/1.1 400 Bad Request', true);
	    die("Bad PID.\n");
    }
    $p_id = intval($data[1]);
    
    // Filetype
    if (array_key_exists(2, $data) && $data[2] == 'listen.xspf')
    {
        $file_type = FILE_TYPE_XSPF;
    }
}
else
{
    header('HTTP/1.1 400 Bad Request', true);
    die("Missing PID.\n");
}

/******************************************************************************/
/*                                  MODEL                                     */
/******************************************************************************/

// Inclusions
include_once(dirname(__FILE__).'/inc/prepend.php');

// Get the servers associated with this mountpoint
$servers = Server::retrieveByMountpointId($p_id);

// Build the playlist
$playlist = array();
if ($servers !== false && $servers !== array())
{
    foreach ($servers as $s)
    {
		$playlist[] = $s->getListenUrl();
    }
}		
else
{
    header('HTTP/1.1 404 Not Found', true);
    die("No such PID.\n");
}

// Logging
try
{
    $mountpoint = Mountpoint::retrieveByPk($p_id);
    if ($mountpoint instanceOf Mountpoint)
    {
        $sn = $mountpoint->getStreamName();
        statsLog::playlistAccessed($p_id, $sn);
    }
}
catch (SQLException $e)
{
    // Nothing to do, it's just logging after all...
}

/******************************************************************************/
/*                                     VIEW                                   */
/******************************************************************************/

switch ($file_type)
{
    case FILE_TYPE_M3U:
        // Header
        header("Content-type: audio/x-mpegurl");
        if (preg_match("/MSIE 5.5/", $_SERVER['HTTP_USER_AGENT']))
        {
	        header("Content-Disposition: filename=\"listen.m3u\"");
        }
        else
        {
	        header("Content-Disposition: inline; filename=\"listen.m3u\"");
        }

        // Data
        echo implode("\r\n", $playlist);
        break;
    case FILE_TYPE_XSPF:
        // Header
        header("Content-type: application/xspf+xml");
        if (preg_match("/MSIE 5.5/", $_SERVER['HTTP_USER_AGENT']))
        {
	        header("Content-Disposition: filename=\"listen.xspf\"");
        }
        else
        {
	        header("Content-Disposition: inline; filename=\"listen.xspf\"");
        }
        
        // Data
        $mountpoint = Mountpoint::retrieveByPk($p_id);
        printf('<?xml version="1.0" encoding="UTF-8"?>
<playlist version="1" xmlns="http://xspf.org/ns/0/">
    <title>%s</title>
    <info>%s</info>
    <trackList>
',
        $mountpoint->getStreamName(),
        $mountpoint->getUrl());
        foreach ($playlist as $p)
        {
            printf("        <track><location>%s</location></track>\n", $p);
        }
        echo '    </trackList>
</playlist>
';
        break;
}

?>
