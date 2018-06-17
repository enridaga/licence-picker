<?php
require_once  'default.inc';
class PickerController extends DefaultController {
	public function __construct($args = array()){
		parent::__construct($args);
	}
	
	public function indexAction(){
		$this->pickerAction();
	}
	
	public function rebuildAction(){
		DataAccess::rebuild();
		$this->_success('The knowledge base has been rebuilt.');
		header('Location: ' . route('default', 'index'), 201);
	}
	
	public function pickerAction(){
		$d = DataAccess::instance();
		
		// top and bottom
		$top = $d->top();
		$bottom = $d->bottom();
		
		// initialize yes questions (get concepts from ids)		
		$yesq = array(); // contains yes concepts
		if($this->has('yes')) {
			$yes = $this->get('yes'); // contains yes conceptsIds
			foreach($yes as $y){
				array_push($yesq, $d->question($y));
			}
		}else{
			$yes = array();
		}
		
		// initialize no questions (get concepts from ids)
		$noq = array(); // contains no concepts
		if($this->has('no')) {
			$no = $this->get('no'); // contains no conceptsId
			foreach($no as $y){
				array_push($noq, $d->question($y));
			}
		}else{
			$no = array();
		}
		
		// initialized actions stack (history)
		if($this->has('stack')){
			$stack = $this->get('stack');
		}else{
			$stack = array();
			/*
			 * stack is a list of yes/no actions performed, 
			 * so that we know from which list we have to remove
			 * the last element when the user performs undo
			 */
		}
		
		// perform sat yes action and set current node/concept
		if($this->has('sayyes')) {
			// add action to stack (history)
			array_push($stack,'sayyes');
			// set current question (later we'll branch from here)
			$current = $d->question($this->get('sayyes'));
			// add current concept to yes list
			array_push($yes, $current['concept']); // id
			array_push($yesq, $current); // full desc
		} else if($this->has('current')) {
			// if other action, set current concept
			$current = $d->question($this->get('current'));
		} else {
			// by default
			$current = $d->top();
		}
		
		// perform say no action
		if($this->has('sayno')) {
			// add to stack
			array_push($stack, 'sayno');
			array_push($no, $this->get('sayno'));
			array_push($noq, $d->question($this->get('sayno')));
		}

		// perform undo action
		if($this->has('undo')){
			/*
			 * order is important, we modify here variables alrady set
			 */
			$undo = array_pop($stack);
			if($undo == 'sayyes'){
				array_pop($yes);
				array_pop($yesq);
				if(count($yesq)>0){
					$current = $yesq[count($yesq)-1];
				} else {
					$current = $top;
				}
			}else if($undo == 'sayno'){
				array_pop($no);
				array_pop($noq);
			}
		}
		
		$q = $this->_prepareQuestions($current, $yes, $no, $top, $bottom);
		$lyes = $this->_prepareLicences($yes, $no);
		
		// attach policies to the licences to display
		foreach($lyes as &$l){
			$l['policies'] = array();
			$policies = $d->policies($l['licence']);
			foreach($policies as $type => $rows){
				$l['policies'][$type] = array();
				foreach($rows as $row){
					array_push($l['policies'][$type], $this->prefixed($row[$type]));
				}
			}
		}
		
		$isBottom = ($q[0] == $bottom);
		// Prepare model
		$m = $this->model();
		$m['data'] = array(
			'yes' => $yesq,
			'no' => $noq,
			'questions' =>	$q,
			'licences' => $lyes,
			'current' => $current,
			'stack' => $stack,
			'isBottom' => $isBottom
		);
		$m['page'] = 'picker';
		$v = new MVC_Html($m);
		$v->render('layout');	
	}
	
	private function _prepareQuestions($current, $yes, $no, $top, $bottom){
		$d = DataAccess::instance();
		// Branch
		$q = $d->childrenQuestionsOf($current);
		//print_r($q); die;
		// check if branching to bottom
		$isBottom = ($q[0] == $bottom);
		// In this case check whether there are unanswered
		// questions in one of the immediate parents
// 		if($isBottom){
// 			$q = $d->parentQuestionsOf($current);
// 			//print_r($q); die;
// 			// remove all yes from questions (already answered)
// 			$rq = array(); // indexes of questions to remove
// 			for($i=0; $i<count($q); $i++){
// 				if(in_array($q[$i]['concept'], $yes)){
// 					array_push($rq,$i);
// 				}
// 			}
// 			foreach($rq as $ri){ //do remove
// 				unset($q[$ri]);
// 			}
// 			sort($q); // refresh array indexes
// 		}
		
		// remove all no from questions
		$rq = array(); // indexes of questions to remove
		for($i=0; $i<count($q); $i++){
			if(in_array($q[$i]['concept'], $no)){
				array_push($rq,$i);
			}
		}
		foreach($rq as $ri){ //do remove
			unset($q[$ri]);
		}
		sort($q); // refresh array indexes
		return $q;
	}
	
	private function _prepareLicences($yes, $no){
		$d = DataAccess::instance();
		// setup licences
		$lyes = $d->licences($yes);
		
		// remove all licences in no questions
		$lno = array(); // licences that are instances of no concepts
		foreach($no as $n){
			$nos = $d->licencesOfType($n);
			$lno = array_merge($lno, $nos);
		}
		$rix = array(); // indexes of licences to remove
		$i=0;
		for($i=0; $i<count($lyes); $i++){
			if(in_array($lyes[$i]['licence'], $lno)){
				array_push($rix,$i);
			}
		}
		foreach($rix as $ri){
			unset($lyes[$ri]); //do remove
		}
		sort($lyes); // refresh indexes
		return $lyes;
	}
}