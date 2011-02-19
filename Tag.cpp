#include "Weights.h"
#include "Tagger.h"
#include <HypergraphAlgorithms.h>

#include <iostream>
#include <iomanip>
#include <algorithm>

#include "common.h"
#include <boost/program_options.hpp>

using namespace std;
using namespace Scarab::HG;
namespace po = boost::program_options;


int main(int argc, char ** argv) {
  

  GOOGLE_PROTOBUF_VERIFY_VERSION;

  wvector * weight = load_weights_from_file( argv[1]); //vm["weights"].as< string >().c_str());
  double total_score = 0.0;
  for (int i=atoi(argv[3]); i <= atoi(argv[4]); i++) {  

    stringstream fname;
    fname << argv[2] << i;
    Tagger f(200);// = new DepParser();
    f.build_from_file(fname.str().c_str());
    
    //bool lp = (int)atoi(argv[3]);
    
    
    HypergraphAlgorithms ha(f);
    EdgeCache * edge_weights = ha.cache_edge_weights(*weight);
    
    //cout << "START!!!!" << endl;
    
    NodeCache  score_memo_table(f.num_nodes()); 
    
    NodeBackCache  back_memo_table(f.num_nodes());
    
    double score = ha.best_path( *edge_weights, score_memo_table, back_memo_table);
    
    HNodes best_nodes = ha.construct_best_node_order(back_memo_table);
    
    HEdges best_edges = ha.construct_best_edges(back_memo_table);
    
    cout << endl;
    vector <Tag> res;
    foreach (HNode node, best_nodes) {
      if (f.node_has_tag(*node)) {
        Tag d = f.node_to_tag(*node);
        res.push_back(d);
      }
    }
    sort(res.begin(), res.end());
    cout << "SENT: "; 
    foreach (Tag d, res) {
      cout << d << " ";
    }
    cout << endl;

    
    //foreach (HNode node, best_nodes) { 
    //cout << ((ForestNode *)node)->_label << endl;
    //b}
    cout << "Score is : " << -score << endl;

    total_score += score;
  }

  cout << "Total Score " << -total_score << endl;
  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}
