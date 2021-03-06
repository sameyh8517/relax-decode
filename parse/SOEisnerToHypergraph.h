#ifndef EISNERTOHYPERGRAPH_H_
#define EISNERTOHYPERGRAPH_H_
#define MAX_LEN 200


#include <sstream>
#include <vector>
#include "hypergraph.pb.h"
#include "dep.pb.h"
#include "features.pb.h"
#include "../parse/DepParser.h"
//#include "Hypergraph.h"
//#include "HypergraphImpl.h"
#include <iostream>
using namespace std;

#define DEBUG 1
//using namespace Scarab::HG;


enum Direction {NODIR, LEFT, RIGHT};
enum Shape {TRI, TRAP, BOX};

struct Span {
  int start;
  int end;
  int size;
  Span(int s, int e) :start(s), end(e){
    size = e-s; 
  } 

  string name() {
    stringstream buf;
    buf << " [" << start<< "," << end << "] ";
    return buf.str();
  }

  bool operator==(const Span & other ) const {
    return other.start == start && other.end == end;
  }

  bool operator<(const Span & other ) const {
    if (other.start != start) {
      return start < other.start;
    } else if (other.end != end) {
      return end < other.end;
    } 
    return false;
  }


};

struct EisnerNode {
  Span node_span; 
  Direction d;
  Shape s;
  
  string name() {
    string ret = "";
    switch (d){
    case RIGHT: 
      ret += "RIGHT ";
        break;
    case LEFT: 
      ret += "LEFT ";
    }
    switch (s){
    case BOX: 
      ret += "BOX ";
        break;

    case TRI: 
      ret += "TRI ";
        break;
    case TRAP: 
      ret += "TRAP ";

    }
    ret += node_span.name();
    return ret;
  }


EisnerNode(Span ns, Direction d_in, Shape s_in) : 
  node_span(ns), d(d_in), s(s_in){}

  bool operator<(const EisnerNode & other ) const {
    if (d != other.d ) {
      return d < other.d;
    } else if (s != other.s) {
      return s < other.s;
    } else if (!(node_span == other.node_span)) {
      return node_span < other.node_span;
    }
    return false;
  } 

  bool operator==(const EisnerNode & other) const {
    return (d == other.d) && (s == other.s) && (node_span == other.node_span);
  }

};

struct LocalHyperedge{
  vector <int> tail_node_ids;
  int head;
  double weight;
  string label;
  LocalHyperedge() {
    weight =0.0;
  }  
};

class EisnerToHypergraph {


 public:
 EisnerToHypergraph(const vector <int> & sent,  vector<vector <vector<double > > > & weights) : _sent(sent), _weights(weights)   {
    _id =0; 
    _edge_id =0;
  }

  void convert( Hypergraph & _forest );
  Hypergraph hgraph;

 private:
  inline int length() {
    return _sent.size();
  }

  inline Hypergraph_Node * finalize_node(EisnerNode & enode){
    Hypergraph_Node * node;
    map < EisnerNode, int >::const_iterator check = _node_to_id.find(enode); 

    if (check != _node_to_id.end()) {
      int id = check->second;
      const EisnerNode & match = check->first;
      node = _id_to_proto[id];
      //cerr << "Found " << id << " " << enode.name() << " " << node->label()<< endl;
      assert(node->label() == enode.name());
      assert(match == enode);
    } else {
      int i = _id;
      _node_to_id[enode] = i;
    
      node = hgraph.add_node();
      node->set_id( i);
      node->set_label(enode.name());
      //cerr << "Adding " << _id << " " << enode.name() << " " << node->label()<< endl;
      //cout << "Adding " << enode.name() << " " << i << endl;
      _id_to_proto[i] = node;
      _id++;
    }
    return node;
  }

  inline void finalize_root() {
    EisnerNode n (Span(0, length()-1), RIGHT, TRI);
    int root = _node_to_id[n];
    hgraph.set_root(root);
    //cout << "set root"
  }


  inline Hypergraph_Edge & finalize_edge(Hypergraph_Node * hnode, LocalHyperedge ledge){
    int i = _edge_id;
    hyperedges.push_back(ledge);

    Hypergraph_Edge * edge = hnode->add_edge();
    edge->set_id( i);
    stringstream buf;
    buf << "value=" << ledge.weight;
    edge->SetExtension(edge_fv, buf.str() );
    
    edge->set_label( ledge.label);
   
    for (int j=0; j < ledge.tail_node_ids.size(); j++) {
      edge->add_tail_node_ids(ledge.tail_node_ids[j]);
      if (DEBUG) {
        //cerr << "connecting  " << hnode->id() << " " << hnode->label() << " " << ledge.tail_node_ids[j] << endl;
        assert(hnode->id() != ledge.tail_node_ids[j]);
      }
    }

    
    _edge_id++;
    return *edge;
  }

  
  inline double get_weight(int h, int m1, int m2) {
    

    //cout << "Weights " << h << " " << m << " " << w << endl;  
    return _weights[h][m1][m2];
  }


  const vector <int> & _sent;
  vector <vector <vector<double > > > &   _weights;  
  
  int _id;
  int _edge_id;
  map < EisnerNode, int > _node_to_id;
  map < int, Hypergraph_Node * > _id_to_proto;
  vector < LocalHyperedge > hyperedges;
};

class SecondOrderConverter {
 public:
  vector<DepParser *> convert_file(const char *file);
};

#endif
