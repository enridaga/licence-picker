<?php
require_once  'default.inc';
class OntologyController extends MVC_Controller {
	
	public function indexAction(){
		$d = DataAccess::instance();
		$ns = array(
				'foaf' => 'http://xmlns.com/foaf/0.1/',
				'dc' => 'http://purl.org/dc/elements/1.1/',
				'rdfs' => 'http://www.w3.org/2000/01/rdf-schema#',
				'contento' => 'http://www.example.org/contento/',
				'contento_ns' => 'http://www.example.org/contento/ns/',
				'odrl' => 'http://www.w3.org/ns/odrl/2/',
				'dct' => 'http://purl.org/dc/terms/',
				'cc' => 'http://creativecommons.org/ns#',
				'skos' => 'http://www.w3.org/2004/02/skos/core#',
				'owl' => '<http://www.w3.org/2002/07/owl#>'
		);
		$ser = ARC2::getTurtleSerializer(array('ns' => $ns));
		header("Content-type: text/plain; charset=utf-8");
		print $ser->getSerializedIndex($d->getTriplesIndex(DataAccess::TAXONOMY));
	}
}