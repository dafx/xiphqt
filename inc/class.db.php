<?php
  
/**
 * Database interfaces.
 *
 * @version $Id$
 * @copyright 2005-2007
 * @author Vincent Tabard
 */

/**
* Base database connection class.
* 
* Note: izterator and izteratorBuilder are classes from the Yoop framework
* ({@link http://www.yoop.org/}). Given that izterator implements the interface
* Iterator (from PHP5's SPL), you can easily adapt this file in order not to use
* izterator - but these classes are free software, so it might be easier just to
* download them ;)
* 
* @author Vincent Tabard
*/
abstract class DataBaseConnection
{
	/**
	* Database server host.
	* 
	* @var string
	*/
	private $db_host;
	
	/**
	* Database server username.
	* 
	* @var string
	*/
	private $db_user;
	
	/**
	* Database server password.
	* 
	* @var string
	*/
	private $db_pass;
	
	/**
	* Database on the database server.
	* 
	* @var string
	*/
	private $db_base;
	
	/**
	* Database connection handle.
	* 
	* @var resource
	*/
	protected $db_handle;
	
	/**
	 * Number of rows affected by the last query.
	 */
	public $affected_rows = null;
	
	/**
	* Constructor.
	* 
	* @param string $host
	* @param string $user
	* @param string $pass
	* @param string $base
	*/
	protected function __construct($host, $user, $pass, $base)
	{
		$this->db_host = $host;
		$this->db_user = $user;
		$this->db_pass = $pass;
		$this->db_base = $base;
	}
	
	/**
	* Connects to the database.
	* 
	* @return boolean
	*/
	public function connect()
	{
		// Connexion � la BDD
		$this->db_handle = @mysql_connect($this->db_host, $this->db_user, $this->db_pass);
		if ($this->db_handle === false)
		{
		    throw new SQLConnectionException();
		}
		
		// S�lection de la base
		if (!@mysql_select_db($this->db_base))
		{
			throw new SQLDBSelectException();
		}
		
		return true;
	}
	
	/**
	* Performs a query on the database.
	* 
	* @param string $sql The query to execute.
	* @return resource
	* @access public
	*/
	public function query($sql)
	{
		$qry = @mysql_query($sql);
		
		if ($qry === false)
		{
		    throw new SQLBadQueryException();
		}
		else
		{
			return $qry;
		}
	}
	
	/**
	* Performs a query that should return a single (row of) result, or a
	* boolean (DELETE, UPDATE, ...) query.
	* 
	* @param string $sql The query to execute.
	* @return mixed An izterator on the result row or a boolean.
	* @access public
	*/
	public function singleQuery($sql)
	{
		$qry = $this->query($sql);
		
		if ($qry === true)
		{
		    return true;
		}
		elseif (mysql_num_rows($qry) == 0)
		{
		    throw new SQLNoResultException();
		}
		else
		{
			return new izterator(array(mysql_fetch_array($qry)));
		}
	}
	
	/**
	* Performs a SELECT query on the database. It does not actually mind what
	* kind of query you're trying to do, but it tries to return an izterator
	* over the result set.
	* 
	* @param string $sql The query to execute.
	* @return izterator
	* @access public
	*/
	public function selectQuery($sql)
	{
		$qry = $this->query($sql);
		
		if (@mysql_num_rows($qry) == 0)
		{
		    throw new SQLNoResultException();
		}
		else
		{
			// On construit un it�rateur sur les r�sultats
			$izb = new izteratorBuilder();
			while($row = @mysql_fetch_array($qry))
			{
				$izb->push($row);
			}
			
			return $izb->getIzterator();
		}
	}
	
	/**
	* Performs an INSERT query on the database. It does not actually mind what
	* kind of query you're trying to do, but it tries to return the last inserted
	* ID.
	* 
	* @param string $sql The query to execute.
	* @return mixed An int if the query was successful, (boolean) false else.
	* @access public
	*/
	public function insertQuery($sql)
	{
		$qry = $this->singleQuery($sql);
		
		if($qry === true)
		{
			return @mysql_insert_id($this->db_handle);
		}
		else
		{
			return false;
		}
	}
	
	/**
	* Performs a query on the database that doesn't return anything.
	* 
	* @param string $sql The query to execute.
	* @return boolean
	* @access public
	*/
	public function noReturnQuery($sql)
	{
		$qry = $this->singleQuery($sql);
		
		return (boolean) $qry;
	}
	
	/**
	 * Returns the number of rows affected by the latest query.
	 * 
	 * @return int
	 */
	public function affectedRows()
	{
	    $this->affected_rows = @mysql_affected_rows($this->db_handle);
	    
	    return $this->affected_rows;
	}
	
	/**
	 * Returns an instance of the DatabaseConnection.
	 */
	abstract public static function getInstance($force_new = false, $auto_connect = true);
}

/**
* Defines an SQL Exception, ie an Exception during a database operation.
*/
class SQLException extends Exception
{
	/**
	* Constructor. Takes the message from mysql_error(), and then calls its parent's
	* constructor (Exception::__construct()).
	* 
	* @access public
	*/
	function __construct()
	{
		$this->message = mysql_error();
		
		parent::__construct($this->message);
	}
}

/**
* Defines an SQL Exception <b>on connection</b>, ie that happens
* during the connection (bad credentials, connection lost...).
*/
class SQLOnConnectException extends SQLException
{
}

/**
* Defines an SQL On Connect Exception related to the connection process
* itself, and not to the database selection that takes place afterwards.
*/
class SQLConnectionException extends SQLOnConnectException
{
}

/**
* Defines an SQL On Connect Exception related to the database selection
* process.
*/
class SQLDBSelectException extends SQLOnConnectException
{
}

/**
* Defines an SQL Exception <b>on a query</b>, ie that happens during
* or after a query.
*/
class SQLOnQueryException extends SQLException
{
}

/**
* Defines an SQL On Query Exception caused by a bad SQL query (bad syntax, ...).
*/
class SQLBadQueryException extends SQLOnQueryException
{
}

/**
* Defines an SQL On Query Exception that arises from the lack of result (ie there's
* no answer).
*/
class SQLNoResultException extends SQLOnQueryException
{
}
  
?>
