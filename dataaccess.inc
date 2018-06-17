<?php
class DataAccess {
	private static $_instance = NULL;
	private $_contento;
	private $_ini;
	private $_store;
	const TAXONOMY = "http://www.example.org/contento/taxonomy";
	const ONTOLOGY = "http://www.example.org/contento/ontology";
	const RDFLICENSE = "http://www.example.org/contento/rdflicense";
	const CONTENTO_HAS = 'http://www.example.org/contento/has';
	const CONTENTO_HAS_PROPER = 'http://www.example.org/contento/has-proper';
	const CONTENTO_PROPER_HAS = 'http://www.example.org/contento/proper-has';
	const CONTENTO_ATTRIBUTE = 'http://www.example.org/contento/attribute';
	const CONTENTO_PROPER_ATTRIBUTE = 'http://www.example.org/contento/proper-attribute';
	const RDFS_SUBCLASS_OF = "http://www.w3.org/2000/01/rdf-schema#subClassOf";
	const SKOS_BROADER = "http://www.w3.org/2004/02/skos/core#broader";
	
	private function __construct($ini, $rebuild = FALSE) {
		$this->_ini = $ini;
		$this->init ($rebuild);
	}
	
	public function init($rebuild=false) {
		$this->_store = ARC2::getStore ( $this->config ( 'arc' ) );
		
		$errs = $this->_store->getErrors ();
		if (count ( $errs ) > 0) {
			throw new DataAccessException ( $errs );
		}
		//
		$store = $this->_store;
		if ($rebuild && $store->isSetUp ()) {
			$store->drop ();
		}
		if (! $store->isSetUp ()) {
			try {
				$store->setUp ();
				$parser = ARC2::getTurtleParser ();
				$parser->parse ( dirname ( __FILE__ ) . '/resources/' . $this->_ini ['resources'] ['taxonomy'] );
				if ($parser->getErrors ()) {
					throw new DataAccessException ( $parser->getErrors () );
				}
				$store->insert ( $parser->getTriples (), self::TAXONOMY );
				
				// rewrite triples
				$triples = $parser->getTriples ();
				$toadd = array ();
				for($i = 0, $i_max = count ( $triples ); $i < $i_max; $i ++) {
					$triple = $triples [$i];
					if ($triple ['p'] === self::CONTENTO_HAS) {
						$o = $triple ['o'];
						if($o === '') continue;
						$oparts = explode ( ' ', $o );
						$p = 'http://www.w3.org/ns/odrl/2/' . $oparts [0];
// 						if(!isset($oparts[1])){
// 							print_r($triple);
// 							print_r($oparts);
// 							die;
// 						}
						array_push ( $toadd, array (
								's' => $triple ['s'],
								's_type' => $triple ['s_type'],
								'o' => $oparts [1],
								'o_type' => 'uri',
								'p' => $p 
						) );
						
						// specify this is a licence from the taxonomy
						array_push ( $toadd, array (
							's' => $triple ['s'],
							's_type' => $triple ['s_type'],
							'o' => self::TAXONOMY,
							'o_type' => 'uri',
							'p' => 'http://rdfs.org/ns/void#inDataset'
						) );
					}
				}
				// if it is subclass of
				if ($triple ['p'] === self::RDFS_SUBCLASS_OF) {
					// rewrite with skos narrower
					array_push ( $toadd, array (
					's' => $triple ['s'],
					's_type' => $triple ['s_type'],
					'o' => $triple['o'],
					'o_type' => 'uri',
					'p' => self::SKOS_BROADER
					) );
					
					// materialize transitive closure
					//$subclasses = $this->_transitiveClosure($triples, $triple['s'], $triple['p']);
					
				}
				$store->insert ( $toadd, self::TAXONOMY );
				
				$parser = ARC2::getTurtleParser ();
				$parser->parse ( dirname ( __FILE__ ) . '/resources/' . $this->_ini ['resources'] ['rdflicense'] );
				if ($parser->getErrors ()) {
					throw new DataAccessException ( $parser->getErrors () );
				}
				$store->insert ( $parser->getTriples (), self::RDFLICENSE );
				
				if ($store->getErrors ()) {
					throw new DataAccessException ( $store->getErrors () );
				}
			} catch ( Exception $e ) {
				$store->drop ();
				throw $e;
			}
			
			
		}
		
				
		//$graphs = $this->_store->query ( "select ?l where { ?l <http://rdfs.org/ns/void#inDataset> <http://www.example.org/contento/taxonomy>}" );
		//$r = $this->_store->query ( $q );
// 		$errs = $this->_store->getErrors ();
// 		if (count ( $errs ) > 0) {
// 			throw new DataAccessException ( $errs );
// 		}else{
//  			print_r($r); die;
// 		}
		// $parser = ARC2::getTurtleParser();
		// $parser->parse( dirname(__FILE__) . '/resources/' . $ini['resources']['taxonomy'] );
		// //print dirname(__FILE__) . '/resources/' . $ini['resources.taxonomy'] ;
		// if($parser->getErrors()) {
		// throw new Exception('Parser error!' . print_r($parser->getErrors(), TRUE));
		// }
		// $this->_contento = $parser->getTriples();
	}
	
	public function store(){
		return $this->_store;
	}
	
	public function contento() {
		return $this->_contento;
	}
	
	public function config($section = FALSE) {
		if ($section === FALSE) {
			return $this->_ini;
		} else {
			return $this->_ini [$section];
		}
	}
	
	public function top(){
		$q  = <<<QUERY
		prefix odrl: <http://www.w3.org/ns/odrl/2/>
		prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#>
		PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>
		select ?concept ?label ?question where {
			?concept a <http://www.example.org/contento/Concept> .
			filter(!bound(?parent)) .
			optional {
				?concept rdfs:subClassOf ?parent
			}
			?concept rdfs:comment ?question .
			?concept rdfs:label ?label
	    }
QUERY;
		
		$store = $this->_store;
		$rs = $store->query($q);
		if (!$store->getErrors()) {
		$rows = $rs['result']['rows'];
			return $rows[0];
		}else{
			throw new DataAccessException($store->getErrors());
		}
	}
	

	public function getTriplesIndex($graph){
		$store = $this->_store;
		$q = <<<QUERY

		prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> 
		prefix contento: <http://www.example.org/contento/> 
		prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> 
		prefix contento_ns: <http://www.example.org/contento/ns/> 
		prefix odrl: <http://www.w3.org/ns/odrl/2/> 
		prefix void: <http://rdfs.org/ns/void#> 
		CONSTRUCT { ?S ?P ?O } WHERE { graph <$graph> { ?S ?P ?O . filter(
				?P = rdf:type ||
				?P = rdfs:label ||
				?P = rdfs:comment ||
				?P = rdfs:subClassOf ||
				?P = odrl:permission ||
				?P = odrl:prohibition ||
				?P = odrl:duty
				
				) } } 
QUERY;
		$index = $store->query($q);
		return $index['result'];
	}
	
	public function bottom(){
		$q  = <<<QUERY
		prefix odrl: <http://www.w3.org/ns/odrl/2/>
		prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#>
		PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>
		select ?concept ?label ?question where {
			?concept a <http://www.example.org/contento/Concept> .
			filter(!bound(?child)) .
			optional {
				?child rdfs:subClassOf ?concept
			}
			?concept rdfs:comment ?question .
			?concept rdfs:label ?label
	    }
QUERY;
	
		$store = $this->_store;
		$rs = $store->query($q);
		if (!$store->getErrors()) {
			$rows = $rs['result']['rows'];
			return $rows[0];
		}else{
			throw new DataAccessException($store->getErrors());
		}
	}

	public function childrenQuestionsOf($question){
		$concept = $question['concept'];
		$q  = <<<QUERY
		prefix odrl: <http://www.w3.org/ns/odrl/2/>
		prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#>
		PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>
		select ?concept ?label ?question where {
			?concept a <http://www.example.org/contento/Concept> .
			?concept rdfs:subClassOf <$concept> .
			?concept rdfs:comment ?question .
			?concept rdfs:label ?label
	    }
QUERY;
	
		$store = $this->_store;
		$rs = $store->query($q);
		if (!$store->getErrors()) {
			$rows = $rs['result']['rows'];
			return $rows;
		}else{
			throw new DataAccessException($store->getErrors());
		}
	}
	
	public function parentQuestionsOf($question){
		$concept = $question['concept'];
		$q  = <<<QUERY
		prefix odrl: <http://www.w3.org/ns/odrl/2/>
		prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#>
		PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>
		select ?concept ?label ?question where {
			?concept a <http://www.example.org/contento/Concept> .
			<$concept> rdfs:subClassOf ?concept.
			?concept rdfs:comment ?question .
			?concept rdfs:label ?label
	    }
QUERY;
		
		$store = $this->_store;
		$rs = $store->query($q);
		if (!$store->getErrors()) {
			$rows = $rs['result']['rows'];
			return $rows;
		}else{
			throw new DataAccessException($store->getErrors());
		}
	}
	
	public function questions(){
		
		$q  = <<<QUERY
		prefix odrl: <http://www.w3.org/ns/odrl/2/>
		prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#>
		PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>
		select ?concept ?label ?question where {
			?concept a <http://www.example.org/contento/Concept> .
			?concept rdfs:comment ?question .
			?concept rdfs:label ?label 
	    }
QUERY;

		$store = $this->_store;
		$rs = $store->query($q);
		if (!$store->getErrors()) {
			$rows = $rs['result']['rows'];
			return $rows;
		}else{
			throw new DataAccessException($store->getErrors());
		}
	}
	
	public function question($uri){
	
		$q  = <<<QUERY
		prefix odrl: <http://www.w3.org/ns/odrl/2/>
		prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#>
		PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>
		select ?label ?question where {
			<$uri> a <http://www.example.org/contento/Concept> .
			<$uri> rdfs:comment ?question .
			<$uri> rdfs:label ?label
	    }
QUERY;
	
		$store = $this->_store;
		$rs = $store->query($q);
		if (!$store->getErrors()) {
			$rows = $rs['result']['rows'];
			$r = $rows[0];
			$r['concept'] = $uri;
			return $r;
		}else{
			throw new DataAccessException($store->getErrors());
		}
	}
	
	public function licences($types = array()){
		
		$fyes = '';
		foreach($types as $y):
			$fyes .= ' ?licence a <' . $y . '> .' . "\n";
		endforeach;
		
		$graph = self::TAXONOMY;
		$q  = <<<QUERY
		prefix odrl: <http://www.w3.org/ns/odrl/2/>
		prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#>
		PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>
		prefix cc: <http://creativecommons.org/ns#> 
		PREFIX void: <http://rdfs.org/ns/void#>
		select ?licence ?label ?link where {
			{
			?licence a odrl:Policy .
			?licence void:inDataset <$graph> .
			$fyes
			?licence rdfs:label ?label .
			optional { ?licence cc:legalcode ?link } 
			}union{
			?licence a odrl:Set .
			?licence void:inDataset <$graph> .
			$fyes
			?licence rdfs:label ?label .
			optional { ?licence cc:legalcode ?link } 
			}
	    }
QUERY;
		
		$store = $this->_store;
		$rs = $store->query($q);
// 		print_r($q); die;
		if (!$store->getErrors()) {
			$rows = $rs['result']['rows'];
			return $rows;
		}else{
			throw new DataAccessException($store->getErrors());
		}
	}
	
	public function policies($licence){
		$policies = array();
		$policies['permission'] = $this->permissions($licence);
		$policies['prohibition'] = $this->prohibitions($licence);
		$policies['duty'] = $this->duties($licence);
		return $policies;
	}

	public function permissions($licence){
		$q  = <<<QUERY
		prefix odrl: <http://www.w3.org/ns/odrl/2/>
		prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#>
		PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>
		prefix cc: <http://creativecommons.org/ns#>
		select ?permission  
		where {
			<$licence> odrl:permission ?permission . filter(isIRI(?permission))
	    }
QUERY;
		$store = $this->_store;
		$rs = $store->query($q);
		if (!$store->getErrors()) {
			$rows = $rs['result']['rows'];
			return $rows;
		}else{
			throw new DataAccessException($store->getErrors());
		}
	}

	public function prohibitions($licence){
		$q  = <<<QUERY
		prefix odrl: <http://www.w3.org/ns/odrl/2/>
		prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#>
		PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>
		prefix cc: <http://creativecommons.org/ns#>
		select ?prohibition where {
			<$licence> odrl:prohibition ?prohibition . filter(isIRI(?prohibition))
	    }
QUERY;
		$store = $this->_store;
		$rs = $store->query($q);
		if (!$store->getErrors()) {
			$rows = $rs['result']['rows'];
			return $rows;
		}else{
			throw new DataAccessException($store->getErrors());
		}
	}
	

	public function duties($licence){
		$q  = <<<QUERY
		prefix odrl: <http://www.w3.org/ns/odrl/2/>
		prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#>
		PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>
		prefix cc: <http://creativecommons.org/ns#>
		select ?duty where {
			<$licence> odrl:duty ?duty . filter(isIRI(?duty))
	    }
QUERY;
		$store = $this->_store;
		$rs = $store->query($q);
		if (!$store->getErrors()) {
			$rows = $rs['result']['rows'];
			return $rows;
		}else{
			throw new DataAccessException($store->getErrors());
		}
	}
	
	
	public function licencesOfType($type){
		$q  = <<<QUERY
		prefix odrl: <http://www.w3.org/ns/odrl/2/>
		prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#>
		PREFIX rdfs: <http://www.w3.org/2000/01/rdf-schema#>
		prefix cc: <http://creativecommons.org/ns#>
		select ?licence where {
			{
			<$type> <http://www.example.org/contento/object> ?licence .
			}
	    }
QUERY;
		
		$store = $this->_store;
		$rs = $store->query($q);
		$licences = array();
		if (!$store->getErrors()) {
			$rows = $rs['result']['rows'];
			foreach($rows as $row):
				array_push($licences, $row['licence']);
			endforeach;
		}else{
			throw new DataAccessException($store->getErrors());
		}
		
		return $licences;
	}
	
	private static function parseIni(){
		return parse_ini_file ( dirname ( __FILE__ ) . '/dataaccess.ini', TRUE );
	}
	
	public static function instance() {
		if (self::$_instance === NULL) {
			self::$_instance = new DataAccess ( self::parseIni() );
		}
		return self::$_instance;
	}
	
	public static function rebuild(){
		self::$_instance = new DataAccess ( self::parseIni(), TRUE );
		return self::instance();
	}
}
class DataAccessException extends Exception {
	private $_arcErrs = array ();
	public function getARCErrors() {
		return $this->_arcErrs;
	}
	public function __construct($errs = '') {
		if (is_array ( $errs )) {
			$this->_arcErrs = $errs;
			$errs = implode ( "\n", $errs );
		}
		parent::__construct ( $errs, 0, $this );
	}
}