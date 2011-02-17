#include "ForestLattice.h"
#include <iostream>
#include <iomanip>
#include <string>
#include "../common.h"
using namespace std;

void ForestLattice::make_proper_graph(const Lattice & lat) {
  vector <Graphnode*> nodes;
  Edges all_edges;
  for (int i = 0; i < lat.node_size(); i++) {
    const Lattice_Node & node =  lat.node(i);
    assert(node.id() == nodes.size());
    nodes.push_back(new Graphnode(node.id()));
  }
  

  for (int i = 0; i < lat.node_size(); i++) {
    const Lattice_Node & node =  lat.node(i);

    Edges local_edges;
    for (int j =0; j < node.edge_size(); j++) {
      const Lattice_Edge & edge = node.edge(j);
      
      Graphedge * local_edge = new Graphedge(edge.id(), *nodes[node.id()], *nodes[edge.to_id()]);
      
      all_edges.push_back(local_edge);
      local_edges.push_back(local_edge);

    }
    nodes[i]->set_edges(local_edges);
  }
  
  Nodes final_nodes;
  foreach ( Graphnode *node, nodes) 
    final_nodes.push_back((Node ) node );
  _proper_graph = new Graph(final_nodes, all_edges);

}


ForestLattice::ForestLattice(const Lattice & lat) {
  num_nodes = lat.node_size();
  num_word_nodes = lat.GetExtension(num_original_ids);
  int num_hyper_edges = lat.GetExtension(num_hypergraph_edges);
  _words.resize(num_word_nodes);
  _is_word.resize(num_word_nodes);
  original_edges.resize(num_hyper_edges);
  edges_original.resize(num_word_nodes);
  
  word_node.resize(num_nodes);
  edge_node.resize(num_nodes);
  ignore_nodes.resize(num_nodes);
  final.resize(num_nodes);
  node_edges.resize(num_nodes);
  graph.resize(num_nodes);
  _edge_by_nodes.resize(num_nodes);
  _edge_label_by_nodes.resize(num_word_nodes);
  _words_lookup.resize(lat.GetExtension(num_original_ids));
  
  _first_words.resize(num_nodes);
  _last_words.resize(num_nodes);
  _last_bigrams.resize(num_nodes);
  bigrams_at_node.resize(num_nodes);
  _last_same.resize(num_word_nodes);

  _lat_word_to_hyp_node.resize(num_word_nodes);
  _hyp_node_to_lat_word.resize(num_word_nodes);

  make_proper_graph(lat);

  for (int i =0; i< num_word_nodes; i++) {
  
    _last_same[i] = -1;
  }
 
 _original_id_to_edge.resize(num_word_nodes);
 //int same =0;
  for (int i = 0; i < lat.node_size(); i++) {
    const Lattice_Node & node =  lat.node(i);


    //cout << node.id()<<endl;
    //assert ((int)_nodes.size() == node.id());
    
    
    node_edges[node.id()] = node.edge_size();
    graph[node.id()].resize(node.edge_size());
    _edge_by_nodes[node.id()].resize(num_nodes);
    //_edge_label_by_nodes[node.id()].resize(num_nodes);


    for (int j =0; j < lat.node_size(); j++) {
      _edge_by_nodes[node.id()][j] = -1;
    }

    if (node.GetExtension(has_phrases)) {
      const Phraselets & plets  = node.GetExtension(phraselets);
      int size = plets.phraselet_size();
      //_first_words[node.id()];
      //_last_words[node.id()]

      for (int i =0; i < size; i ++) {
        const Phraselet & plet= plets.phraselet(i);
        for (int j=0; j < plet.word_size(); j++) {
          const Subword & word = plet.word(j);
          int hyper_edge =plet.phraselet_hypergraph_edge();

          if (word.subword_hypergraph_node_id()!= -1) {
            _lat_word_to_hyp_node[word.subword_original_id()] = word.subword_hypergraph_node_id();
            _hyp_node_to_lat_word[word.subword_hypergraph_node_id()] = word.subword_original_id();
            //cout << word.subword_hypergraph_node_id() << " " << word.subword_original_id() << endl;
          }
          _words_lookup[word.subword_original_id()] = node.id();
          _words[word.subword_original_id()] = word.word();
          _is_word[word.subword_original_id()] = 1;
          if (hyper_edge != -1)
            original_edges[hyper_edge].push_back(word.subword_original_id());
          if (j < plet.word_size() -1) {
            bigrams_at_node[node.id()].push_back(Bigram(word.subword_original_id(),
                                                         plet.word(j+1).subword_original_id()));
          }
          
        }
        assert(plet.word_size() > 0); 
        int last = plet.word_size()-1;
        assert(plet.word(0).subword_original_id() != 0);
        //assert(plet.word(last).subword_original_id() != 0 );
        //cout << "First " << node.id() << " " << plet.word(0).subword_original_id() << endl;
        //cout << "Last " << node.id() << " " << plet.word(last).subword_original_id() << endl;
        _first_words[node.id()].push_back(Word(plet.word(0).subword_original_id()));


        for (uint i =0 ; i < _last_words[node.id()].size(); i++) {
          if (plet.word(last).word() == _words[_last_words[node.id()][i].id()]) {
            // this position is the same as some previous
            _last_same[plet.word(last).subword_original_id()] = _last_words[node.id()][i].id();
            break;
          }
        }

        _last_words[node.id()].push_back(Word(plet.word(last).subword_original_id()));
        
        if (plet.word_size() >= 2) {
          Bigram b(plet.word(last-1).subword_original_id(), 
                   plet.word(last).subword_original_id());
          
          _last_bigrams[node.id()].push_back(b);
        }

        // DEBUG
        /*for (int i =0 ; i < _first_words.size(); i++) {
          if (plet.word(0).word() == _words[_first_words[node.id()][i]]) {
            cout << "SAME WORD";
          }
          }*/

        
      }
    }
    

    for (int j =0; j < node.edge_size(); j++) {
      const Lattice_Edge & edge = node.edge(j);
      
      graph[node.id()][j] = edge.to_id();
      
      string label = edge.label();

      const Origin & orig = edge.GetExtension(origin);
      if (orig.has_origin()) {
        int original_id = orig.original_id();
        for (int k =0; k < orig.hypergraph_edge_size(); k++) {
          int hypergraph_edge = orig.hypergraph_edge(k);
          original_edges[hypergraph_edge].push_back(original_id);
          edges_original[original_id].push_back(hypergraph_edge);
          
        }
        //edges[]
        _edge_by_nodes[node.id()][edge.to_id()] = original_id;
        _edge_label_by_nodes[original_id] = label;
        //cout << "EDGE " << original_id << " " << node.id() << " " << edge.to_id() << endl;
        _original_id_to_edge[original_id] = Bigram(node.id(), edge.to_id());
      } else {
        //cout << "NO EDGE " << node.id() << " " << edge.to_id() << " " << endl;
        _edge_by_nodes[node.id()][edge.to_id()] = -1;
      }

    }


    if (node.GetExtension(has_phrases)) {
      word_node[node.id()] = 1; //node.getExtension(word);
      edge_node[node.id()] = -1;
      //cout << node.GetExtension(word) << endl;
      //cout <<node.id() << endl;
      //_words[node.id()]= node.GetExtension(word);
      _phrase_nodes.push_back(&_proper_graph->node(node.id()));
    } else {
      word_node[node.id()] = -1;
      edge_node[node.id()] = 1;
    }
    //int orig_node =node.GetExtension(original_node);
    //bool ignore = node.GetExtension(ignore_node);
    //if (ignore) {
      //cout << "IGNORING " << node.id() <<endl;
      //assert (orig_node == -1);
    //}
    //if (orig_node != -1) {
    //assert (orig_node >= 0 && orig_node < num_nodes);
    //original_nodes[orig_node].push_back(node.id());
    //}
    
    //ignore_nodes[node.id()] = node.GetExtension(ignore_node);    
  }

  start = lat.start();
  for (int i=0; i < lat.final_size(); i++) {
    final[lat.final(i)] = 1;
  }  


  //cout << "Same " << same << endl;
  //cout << "Words " << num_word_nodes << endl;
} 


