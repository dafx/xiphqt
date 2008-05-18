<?php

// Inclusions
include_once(dirname(__FILE__).'/../inc/prepend.php');

// Database connection
$db = DirXiphOrgLogDBC::getInstance();

function makeFutureDate($format = 'Ymd', $days = 1)
{
    return date($format, mktime(0, 0, 0, date('m'), date('d') + $days, date('Y')));
}
$yesterday = makeFutureDate('Ymd', -1);

/******************************************************************************/
/**                               API LOG                                    **/
/******************************************************************************/
try
{
    printf("Creating logged_at index on api_log_%s... ", $yesterday);
    $start = microtime(true);
    $sql = 'CREATE INDEX `logged_at` ON `api_log_%s` (`logged_at`);';
    $sql = sprintf($sql, $yesterday);
    $db->query($sql);
    printf("OK [%.3f s]\n", microtime(true) - $start);
}
catch (SQLException $e)
{
    echo "[ERROR]\n";
    
    // Mail the error...
    custom_exception_handler($e);
    
    // do nothing.
}
try
{
    printf("Creating message index on api_log_%s... ", $yesterday);
    $start = microtime(true);
    $sql = 'CREATE INDEX `message` ON `api_log_%s` (`message`, `logged_at`);';
    $sql = sprintf($sql, $yesterday);
    $db->query($sql);
    printf("OK [%.3f s]\n", microtime(true) - $start);
}
catch (SQLException $e)
{
    echo "[ERROR]\n";
    
    // Mail the error...
    custom_exception_handler($e);
    
    // do nothing.
}

echo "Creating future tables... ";
$sql_pattern = 'CREATE TABLE IF NOT EXISTS `api_log_%s` LIKE `api_log_template`;';
for ($i = 0 ; $i <= 7 ; $i++)
{
    $date = makeFutureDate('Ymd', $i);
    echo $date." ";
    
    $sql = sprintf($sql_pattern, $date);
    try
    {
        $db->query($sql);
        echo "[OK]... ";
    }
    catch (SQLException $e)
    {
        echo "[ERROR] ! ";
        // Mail the error...
        custom_exception_handler($e);
        
        // do nothing.
    }
}
echo "\n";

/******************************************************************************/
/**                              PLAYLIST LOG                                **/
/******************************************************************************/
try
{
    printf("Creating stream_name_hash index on playlist_log_%s... ", $yesterday);
    $start = microtime(true);
    $sql = 'CREATE INDEX `stream_name_hash` ON `playlist_log_%s` (`stream_name_hash`);';
    $sql = sprintf($sql, $yesterday);
    $db->query($sql);
    printf("OK [%.3f s]\n", microtime(true) - $start);
}
catch (SQLException $e)
{
    echo "[ERROR]\n";
    
    // Mail the error...
    custom_exception_handler($e);
    
    // do nothing.
}

echo "Creating future tables... ";
$sql_pattern = 'CREATE TABLE IF NOT EXISTS `playlist_log_%s` LIKE `playlist_log_template`;';
for ($i = 0 ; $i <= 7 ; $i++)
{
    $date = makeFutureDate('Ymd', $i);
    echo $date." ";
    
    $sql = sprintf($sql_pattern, $date);
    try
    {
        $db->query($sql);
        echo "[OK]... ";
    }
    catch (SQLException $e)
    {
        echo "[ERROR] ! ";
        // Mail the error...
        custom_exception_handler($e);
        
        // do nothing.
    }
}
echo "\n";

/******************************************************************************/
/**                             REFUSED LOG                                  **/
/******************************************************************************/
try
{
    printf("Creating reason index on refused_log_%s... ", $yesterday);
    $start = microtime(true);
    $sql = 'CREATE INDEX `reason` ON `refused_log_%s` (`reason`);';
    $sql = sprintf($sql, $yesterday);
    $db->query($sql);
    printf("OK [%.3f s]\n", microtime(true) - $start);
}
catch (SQLException $e)
{
    echo "[ERROR]\n";
    
    // Mail the error...
    custom_exception_handler($e);
    
    // do nothing.
}
try
{
    printf("Creating hostname index on refused_log_%s... ", $yesterday);
    $start = microtime(true);
    $sql = 'CREATE INDEX `hostname` ON `refused_log_%s` (`listen_url`(11));';
    $sql = sprintf($sql, $yesterday);
    $db->query($sql);
    printf("OK [%.3f s]\n", microtime(true) - $start);
}
catch (SQLException $e)
{
    echo "[ERROR]\n";
    
    // Mail the error...
    custom_exception_handler($e);
    
    // do nothing.
}

echo "Creating future tables... ";
$sql_pattern = 'CREATE TABLE IF NOT EXISTS `refused_log_%s` LIKE `refused_log_template`;';
for ($i = 0 ; $i <= 7 ; $i++)
{
    $date = makeFutureDate('Ymd', $i);
    echo $date." ";
    
    $sql = sprintf($sql_pattern, $date);
    try
    {
        $db->query($sql);
        echo "[OK]... ";
    }
    catch (SQLException $e)
    {
        echo "[ERROR] ! ";
        // Mail the error...
        custom_exception_handler($e);
        
        // do nothing.
    }
}
echo "\n";

/******************************************************************************/
/**                             SEARCH LOG                                   **/
/******************************************************************************/
try
{
    printf("Creating search_type index on search_log_%s... ", $yesterday);
    $start = microtime(true);
    $sql = 'CREATE INDEX `search_type` ON `search_log_%s` (`search_type`);';
    $sql = sprintf($sql, $yesterday);
    $db->query($sql);
    printf("OK [%.3f s]\n", microtime(true) - $start);
}
catch (SQLException $e)
{
    echo "[ERROR]\n";
    
    // Mail the error...
    custom_exception_handler($e);
    
    // do nothing.
}

echo "Creating future tables... ";
$sql_pattern = 'CREATE TABLE IF NOT EXISTS `search_log_%s` LIKE `search_log_template`;';
for ($i = 0 ; $i <= 7 ; $i++)
{
    $date = makeFutureDate('Ymd', $i);
    echo $date." ";
    
    $sql = sprintf($sql_pattern, $date);
    try
    {
        $db->query($sql);
        echo "[OK]... ";
    }
    catch (SQLException $e)
    {
        echo "[ERROR] ! ";
        // Mail the error...
        custom_exception_handler($e);
        
        // do nothing.
    }
}
echo "\n";

/******************************************************************************/
/**                                SID LOG                                   **/
/******************************************************************************/
try
{
    printf("Creating listen_url_hash index on sid_log_%s... ", $yesterday);
    $start = microtime(true);
    $sql = 'CREATE INDEX `listen_url_hash` ON `sid_log_%s` (`listen_url_hash`);';
    $sql = sprintf($sql, $yesterday);
    $db->query($sql);
    printf("OK [%.3f s]\n", microtime(true) - $start);
}
catch (SQLException $e)
{
    echo "[ERROR]\n";
    
    // Mail the error...
    custom_exception_handler($e);
    
    // do nothing.
}

echo "Creating future tables... ";
$sql_pattern = 'CREATE TABLE IF NOT EXISTS `sid_log_%s` LIKE `sid_log_template`;';
for ($i = 0 ; $i <= 7 ; $i++)
{
    $date = makeFutureDate('Ymd', $i);
    echo $date." ";
    
    $sql = sprintf($sql_pattern, $date);
    try
    {
        $db->query($sql);
        echo "[OK]... ";
    }
    catch (SQLException $e)
    {
        echo "[ERROR] ! ";
        // Mail the error...
        custom_exception_handler($e);
        
        // do nothing.
    }
}
echo "\n";

?>
