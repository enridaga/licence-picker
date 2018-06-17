<?php
class DefaultController extends MVC_Controller {
	private $_prefix = array(
		'http://www.w3.org/ns/odrl/2/' => 'odrl',
		'http://creativecommons.org/ns#' => 'cc',
		'http://purl.oclc.org/NET/ldr/ns#'=>'ldr-oclc',
		'http://purl.org/NET/ldr/ns#' => 'ldr',
		'http://www.editeur.org/onix-pl/'=>'onix',
		
	);
	protected $_model = NULL;
	public function __construct($args = array()){
		parent::__construct($args);
		session_start();
		
		// XXX Not implemented yet
		$_SESSION['userid'] = 0;
		
		// Setup model
		$this->_model = array(
			'title' => '',
			'nav' => array(
				
			),
			'page' => 'index',
			'data' => array()
		);
	}
	
	protected function _userid(){
		return $_SESSION['userid'];
	}
	
	public function model(){
		return $this->_model;
	}
	
	protected function _forbidden($reason = ''){
		header('HTTP/1.0 403 Forbidden');
		$m = $this->model();
		$m['title'] = '';
		$m['nav'] = array();
		$m['actions'] = array();
		$m['data']['user'] = $m['user'];
		$m['data']['reason'] = $reason;
		$m['page'] = 'sorry';
		$v = new MVC_Viewer($m);
		$v->render('layout');
		die;
	}
	
	public function prefix($ns){
		if(isset($this->_prefix[$ns])){
			return $this->_prefix[$ns];
		}
		return false;
	}
	
	public function prefixed($uri){
		$ns = $this->guessNamespace($uri);
		if($prefix = $this->prefix($ns)){
			return $prefix . ':' . $this->guessName($uri);
		}
		return $uri;
	}
	
	public function guessName ($uri) {
		return substr($uri,$this->getNamespaceEnd($uri));
	}
	
	public function guessNamespace ($uri) {
		$l = $this->getNamespaceEnd($uri);
		return $l > 1 ? substr($uri ,0, $l) : "";
	}
	
	public function getNamespaceEnd ($uri) {
		$l = strlen($uri)-1;
		do {
			$c = substr($uri, $l, 1);
			if($c == '#' || $c == ':' || $c == '/')
				break;
			$l--;
		} while ($l >= 0);
		$l++;
		return $l;
	}
	
	public function indexAction(){
		$m = $this->model();
		$v = new MVC_Viewer($m);
		$v->render('layout');
	}	
	
	protected function _message($type, $text){
		if(!isset($_SESSION['messages']) || !is_array($_SESSION['messages'])){
			$_SESSION['messages'] = array();
		}
		if(!isset($_SESSION['messages'][$type]) || !is_array($_SESSION['messages'][$type])){
			$_SESSION['messages'][$type] = array();
		}
		array_push($_SESSION['messages'][$type], $text);
	}

	protected function _info($t){
		$this->_message('info', $t); 
	}
	protected function _warning($t){
		$this->_message('warning', $t); 
	}
	protected function _danger($t){
		$this->_message('danger', $t); 
	}
	protected function _success($t){
		$this->_message('success', $t); 
	}
}