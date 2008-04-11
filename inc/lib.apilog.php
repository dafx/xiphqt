<?php

class APILog
{
    public static function log($result, $listen_url = null,
                               $server_id = null, $mountpoint_id = null)
    {
        $db = DirXiphOrgDBC::getInstance();
        $sql = 'INSERT INTO `api_log` (`message`, `remote_ip`, `listen_url_hash`, `server_id`, `mountpoint_id`) '
              .'VALUES ("%s", INET_ATON("%s"), %u, %d, %d);';
        $sql = sprintf($sql, mysql_real_escape_string($result),
                             array_key_exists('REMOTE_ADDR', $_SERVER)
                                ? $_SERVER['REMOTE_ADDR'] : '127.0.0.1',
                             $listen_url !== null ? sprintf('%u', crc32($listen_url)) : 0,
                             $server_id, $mountpoint_id);
        $db->noReturnQuery($sql);
    }
    
    public static function serverAdded($ok, $server_id, $mountpoint_id,
                                       $listen_url)
    {
        self::log($ok ? 'Server added' : 'Server add failed',
                  $listen_url, $server_id, $mountpoint_id);
    }
    
    public static function serverRemoved($ok, $server_id, $mountpoint_id,
                                         $listen_url)
    {
        self::log($ok ? 'Server removed' : 'Server removal failed',
                  $listen_url, $server_id, $mountpoint_id);
    }
    
    public static function mountpointAdded($ok, $mountpoint_id,
                                           $listen_url = null)
    {
        self::log($ok ? 'Mountpoint added' : 'Mountpoint add failed',
                  $listen_url, null, $mountpoint_id);
    }
    
    public static function mountpointRemoved($ok, $mountpoint_id,
                                             $listen_url = null)
    {
        self::log($ok ? 'Mountpoint removed' : 'Mountpoint removal failed',
                  $listen_url, null, $mountpoint_id);
    }
    
    public static function request($req_type, $ok, $listen_url = null,
                                   $server_id = null, $mountpoint_id = null)
    {
        $message = '';
        switch ($ok)
        {
            case REQUEST_OK:
                $message = strtoupper($req_type) . ' OK';
                break;
            case REQUEST_FAILED:
                $message = strtoupper($req_type) . ' failed';
                break;
            case REQUEST_REFUSED:
                $message = strtoupper($req_type) . ' refused';
                break;
        }
        self::log($message, $listen_url, $server_id, $mountpoint_id);
    }
    
    public static function serverRefused($reason, $listen_url = false)
    {
        $db = DirXiphOrgDBC::getInstance();
        $sql = 'INSERT INTO `refused_log` (`reason`, `remote_ip`, `listen_url`, `listen_url_hash`) '
              .'VALUES (%d, INET_ATON("%s"), "%s", %u);';
        $sql = sprintf($sql, intval($reason),
                             array_key_exists('REMOTE_ADDR', $_SERVER)
                                ? $_SERVER['REMOTE_ADDR'] : '127.0.0.1',
                             $listen_url != false ? mysql_real_escape_string($listen_url) : '',
                             $listen_url != false ? sprintf('%u', crc32($listen_url)) : 0);
        $db->noReturnQuery($sql);
    }
}

?>
