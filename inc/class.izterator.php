<?php

/**
* Izterator - implements the PHP5 interface "Iterator".
* 
* @author Lo�c Mathaud
* @package izterator
*/
class izterator implements Iterator
{
	var $array_data;
	var $array_index;
	var $nbResults;

	/**
	 * Constructeur
	 * 
	 * Affecte le tableau pass� en param�tre � une variable de classe puis
	 * appelle la m�thode de classe {@link rewind()}
	 * 
	 * @param array $array tableau de r�sultat MySQL depuis classe dbMysql
	 * @access public 
	 */
	function izterator($array)
	{
		if (is_array($array))
		{
			$this->array_data = $array;
		} 
		$this->nbResults = count($array);
		$this->rewind();
	} 

	/**
	 * Retourne la valeur du champ sp�cifi� par $field pour l'index courant
	 * ou retourne un tableau de tout l'enregistrement pour ce m�me index
	 * 
	 * @param string $field champ � afficher pour l'enregistrement courant
	 * @access public 
	 */
	function current($field = '')
	{
		if (empty($field))
		{
			return $this->array_data[$this->array_index];
		} 
		else
		{
			//$field = strtolower($field);
			return $this->array_data[$this->array_index][$field];
		} 
	} 

	/**
	 * Avance le curseur sur l'enregistrement suivant
	 * 
	 * @access public 
	 */
	function next()
	{
		$this->array_index++;
		return true;
	} 

	/**
	 * Retourne TRUE si on est � la fin de la pile, FALSE sinon
	 * 
	 * @access public 
	 */
	function endof()
	{
		if (($this->array_index + 1) <= $this->nbResults)
		{
			return false;
		} 
		else
		{
			return true;
		} 
	} 

	/**
	 * Retourne l'index point� actuellement
	 * 
	 * @access public 
	 */
	function key()
	{
		return $this->array_index;
	} 

	/**
	 * Fait pointer le curseur sur le premier enregistrement
	 * 
	 * @access public 
	 */
	function rewind()
	{
		$this->array_index = 0;
		return true;
	} 

	/**
	 * Fait pointer le curseur sur le dernier enregistrement
	 * 
	 * @access public 
	 */
	function end()
	{
		$this->array_index = $this->nbResults - 1;
		return true;
	} 

	/**
	 * Retourne le nombre d'enregistrements dans la pile
	 * 
	 * @access public 
	 */
	function count()
	{
		return $this->nbResults;
	} 

	/**
	 * Pour la validit� de l'interface.
	 * 
	 * @access public 
	 */
	function valid()
	{
		return !$this->endof();
	} 
} 

?>
