#include <cy_svector.hpp>
#include <svector.hpp>
#include "Forest.h"
#include "LPBuilder.h"

#include "ForestAlgorithms.h"
#include <sstream>
using namespace std;


#define VAR_TYPE GRB_CONTINUOUS
//#define VAR_TYPE GRB_BINARY

struct LatticeVars {
  vector < vector < vector < GRBVar > > > all_pairs_vars;
  vector < vector < GRBVar > > all_pairs_exist_vars;
  vector < vector < vector < bool > > > has_all_pairs_var;
  string name;
  
  LatticeVars(string n):name(n) {

  }

  void  initialize_all_pairs(const GraphDecompose & gd,
                             const ForestLattice & _lattice,
                             GRBModel * model);
    void add_all_pairs_constraints(const GraphDecompose & gd,
                                   const ForestLattice & _lattice,
                                   GRBModel * model);

};

void  LatticeVars::initialize_all_pairs(const GraphDecompose & gd,
                                        const ForestLattice & _lattice,
                                        GRBModel * model) {

  all_pairs_exist_vars.resize(_lattice.num_nodes); 
  all_pairs_vars.resize(_lattice.num_nodes); 
  has_all_pairs_var.resize(_lattice.num_nodes);  

  for (int i = 0; i < _lattice.num_nodes; i++) {
    all_pairs_vars[i].resize(_lattice.num_nodes);
    all_pairs_exist_vars[i].resize(_lattice.num_nodes);
    has_all_pairs_var[i].resize(_lattice.num_nodes);
    for (int j = 0; j < _lattice.num_nodes; j++) {
      
        has_all_pairs_var[i][j].resize(_lattice.num_nodes);        
        for (int k =0; k < _lattice.num_nodes ; k++) {
          has_all_pairs_var[i][j][k] = false;
        }
        if (!gd.path_exists(i,j)) continue;
        
        all_pairs_vars[i][j].resize(_lattice.num_nodes);


        vector <int> * path = gd.get_path(i, j);
        //cout << i << " " << j << endl;
        assert (path->size() != 0 || i == j);

        for (int k = 0; k < path->size(); k++) {
          stringstream buf;
          buf << name <<" SHORTEST " << i << " " << j << " " << (*path)[k];
          double obj= 0.0; // (j == (*path)[k])? 1.0: 0.0; 
          all_pairs_vars[i][j][(*path)[k]] = model->addVar(0.0, 1.0, obj, VAR_TYPE ,  buf.str());
          has_all_pairs_var[i][j][(*path)[k]] = true;
          //cout << i << " " << j << " " << (*path)[k] << endl; 
        }
        //cout << "EXIST " << i << " " << j << endl;
        stringstream buf;
        buf << name <<" EXIST " << i << " " << j;
        
        all_pairs_exist_vars[i][j] = model->addVar(0.0, 1.0, 0.0 , VAR_TYPE ,  buf.str());
      }
    }
    
    model->update();
}

void LatticeVars::add_all_pairs_constraints(const GraphDecompose & gd,
                                            const ForestLattice & _lattice,
                                            GRBModel * model) {
  for (int i = 0; i < _lattice.num_nodes; i++) { 
    for (int j = 0; j < _lattice.num_nodes; j++) {
      if (!gd.path_exists(i,j)) continue;
      if (i == j) continue; 
      {
        GRBLinExpr sum;
        vector <int> * path = gd.get_path(i, j);
        bool has = false;
        for (int k = 0; k < path->size(); k++) {
          int last = (*path)[k];
          //if (_lattice.is_phrase_node(last)) {
          //model->addConstr(all_pairs_vars[i][j][last] == 0.0);
          //}
          //cout << i << " " << j << " " << last << endl;
          assert(has_all_pairs_var[i][j][last]);
          sum += all_pairs_vars[i][j][last];
          //model->addConstr(all_pairs_exist_vars[i][last] == 1.0);
          //model->addConstr(all_pairs_vars[i][j][last] == all_pairs_path)
          has = true;
        }
        assert (has);
        model->addConstr(all_pairs_exist_vars[i][j] == sum);
      }        
    }
  }
  
    

  // Node constraints (INNER)
  //cout << "INNER" << endl;
  for (int i = 0; i < _lattice.num_nodes; i++) { 
    for (int j = 0; j < _lattice.num_nodes; j++) {
      if (!gd.path_exists(i,j)) continue;
      
      GRBLinExpr sum;
      bool has = false;
      
      if (!_lattice.is_phrase_node(j)) {
        for (int k = 0; k < _lattice.num_nodes; k++) {
          if (!has_all_pairs_var[i][k][j]) continue;
          if (k == j) continue;
          //cout << has_all_pairs_var[i][k][j] << " " << i << " " << k << " " << j << endl;
          
          sum += all_pairs_vars[i][k][j];
          has = true;
        }
      }
      if (!_lattice.is_phrase_node(i)) {
        for (int k = 0; k < _lattice.num_nodes; k++) {
          if (!has_all_pairs_var[k][j][i]) continue;        
          if (i == j) continue;
          //cout << k << " " << j << " " << i << endl;
          sum += all_pairs_vars[k][j][i];
          has = true;
        }
      }
      // below 
      //cout << i << " " << j << endl;
      if  (has) {
        model->addConstr(all_pairs_exist_vars[i][j] == sum);
      }
      
    }
  }
  model->update();
}

void LPBuilder::initialize_word_pairs(Ngram &lm, 
                                      const Cache <LatNode, int> & word_cache,
                                      const GraphDecompose & gd, 
                                      vector < GRBVar > & word_used_vars,
                                      vector < vector < GRBVar >  > & word_pair_vars,
                                      vector < vector < vector < GRBVar > > > & word_tri_vars) {

  word_used_vars.resize(_lattice.num_word_nodes); 
  word_pair_vars.resize(_lattice.num_word_nodes); 
  //vector < vector < bool > > has_pair_var(_lattice.num_word_nodes);
  word_tri_vars.resize(_lattice.num_word_nodes); 


  for (int i = 0; i < _lattice.num_word_nodes; i++) {
    if (!_lattice.is_word(i)) continue; 
    word_pair_vars[i].resize(_lattice.num_word_nodes);
    word_tri_vars[i].resize(_lattice.num_word_nodes);
    stringstream buf;
    buf << "UNI " << i ;
    word_used_vars[i] = model->addVar(0.0, 1.0, 0.0 /*Obj*/, VAR_TYPE /*cont*/,  buf.str()/*names*/);      
  }
  
  for (unsigned int i=0; i< gd.valid_bigrams.size() ;i++) {
    Bigram b = gd.valid_bigrams[i];
    
    //has_pair_var[i].resize(_lattice.num_word_nodes);
    //word_tri_vars[i].resize(_lattice.num_word_nodes);
    stringstream buf;
    buf << "BI " << b.w1 << " " << b.w2;
    //has_pair_var[i][j] = true;
    //cout << "WORD " << i <<  " " << j << endl;
    //VocabIndex context [] = {word_cache.store[b.w2], Vocab_None};
    //double prob = lm.wordProb(word_cache.store[b.w1], context);
    //if (b.w1 == 1 && b.w2==0) prob = 0.0;
    //if (isinf(prob)) prob = -1000000.0;
    
    word_pair_vars[b.w1][b.w2] = model->addVar(0.0, 1.0, 0.0 /*Obj*/, VAR_TYPE /*cont*/,  buf.str()/*names*/);
    word_tri_vars[b.w1][b.w2].resize(_lattice.num_word_nodes);
    for (int m =0; m < gd.forward_bigrams[b.w2].size(); m++) {
      int k = gd.forward_bigrams[b.w2][m];
      stringstream buf;
      buf << "TRI " << b.w1 << " " << b.w2 << " " << k;
      VocabIndex context [] = {word_cache.store[b.w2], word_cache.store[k], Vocab_None};
      double prob = (-0.141221) * lm.wordProb(word_cache.store[b.w1], context);
      if (isinf(prob)) prob = 1000000.0;
      //cout << b.w1 << " " << b.w2 << " " << k << endl;
      word_tri_vars[b.w1][b.w2][k] = model->addVar(0.0, 1.0, prob/*Obj*/, VAR_TYPE /*cont*/,  buf.str()/*names*/);
    }
  }
  model->update();
}

void LPBuilder::build_all_pairs_lp(Ngram &lm, 
                                   const Cache <LatNode, int> & word_cache,
                                   vector < GRBVar > & word_used_vars,
                                   vector < vector < vector < GRBVar > > > & word_tri_vars,
                                   LatticeVars & lv,
                                   const GraphDecompose & gd) {
  
  vector < vector < GRBVar > >  word_pair_vars;
  
  try{
    lv.initialize_all_pairs(gd, _lattice, model);
    initialize_word_pairs(lm, word_cache, gd, word_used_vars, word_pair_vars, word_tri_vars);    
    model->update();

    //cout << "START" << endl;
    // Last must hit first
    //model->addConstr(all_pairs_exist_vars[_lattice.num_nodes-1][0] == 1.0);
    
    //cout << "OUTER" << endl;
    // Node constraints (Outer)
  
    lv.add_all_pairs_constraints(gd, _lattice, model);

    for (int i = 0; i < _lattice.num_nodes; i++) { 
      for (int j = 0; j < _lattice.num_nodes; j++) {
        if (!gd.path_exists(i,j)) continue;
        if (i == j) continue; 
        if (_lattice.is_phrase_node(i) && _lattice.is_phrase_node(j)) {
          GRBLinExpr sum;
          for (int m =0; m < _lattice.num_last_words(i); m++) {
            
            int lword = _lattice.last_words(i, m);
            assert(_lattice.is_word(lword));
            for (int n =0; n < _lattice.num_first_words(j); n++) {
              int rword = _lattice.first_words(j, n);
              assert(_lattice.is_word(rword));
              //assert(has_pair_var)
              //cout << lword << " " << rword << endl;
              sum += word_pair_vars[lword][rword];
            }
          }
          //if (i == j) continue;
          //cout << " " << i <<  " " << j << endl;
          model->addConstr(sum == lv.all_pairs_exist_vars[i][j]);
        }
      }
    }

    /*cout << "INNER" << endl;
    // Node constraints (OUTER)
    for (int i = 0; i < _lattice.num_nodes; i++) { 
      if (_lattice.is_phrase_node(i)) continue;
      for (int j = 0; j < _lattice.num_nodes; j++) {
        if (i == j) continue; 
        if (!gd.path_exists(i,j)) continue;
        
        {
          GRBLinExpr sum;
          bool has = false;
          for (int k = 0; k < _lattice.num_nodes; k++) {
            if (!has_all_pairs_var[k][j][i]) continue;        
            cout << k << " " << j << " " << i << endl;
            sum += all_pairs_vars[k][j][i];
            has = true;
          }

          // below 
          if (has) {
            model->addConstr(all_pairs_exist_vars[i][j] == sum);
          }
        }   
      }
      }*/
    


    // Word constraints    
    //cout << "WORDS" << endl;
    for (int i = 0; i < _lattice.num_word_nodes; i++) {
      if (!_lattice.is_word(i)) continue ;      
      {
        GRBLinExpr sumFor, sumBack;
        for (int j = 0; j < gd.forward_bigrams[i].size(); j++) {
          int next = gd.forward_bigrams[i][j];
          sumFor += word_pair_vars[i][next];
        }
        
        for (int j = 0; j < gd.backward_bigrams[i].size(); j++) {
          int next = gd.backward_bigrams[i][j];
          sumBack += word_pair_vars[next][i];
        }

        if (i != 0) {
          model->addConstr(word_used_vars[i] == sumFor);
        } else {
          model->addConstr(word_used_vars[i] == 1);
        }
      
        if (i != _lattice.num_word_nodes - 1) {
          model->addConstr(word_used_vars[i] == sumBack);
        } else {
          model->addConstr(word_used_vars[i] == 1);
        }
      }

      /*for (int j = 0; j < _lattice.num_word_nodes; j++) {
        if (!_lattice.is_word(j)) continue ;
        
        GRBLinExpr sumFor, sumBack;
        for (int k =0; k < _lattice.num_word_nodes; k++) {
          sumFor += word_tri_vars[i][j][k];
          sumBack += word_tri_vars[k][i][j];
        }
        if (i != 0) {
          model->addConstr(word_pair_vars[i][j] == sumFor);
        }
      
        if (i != _lattice.num_word_nodes - 1) {
          model->addConstr(word_pair_vars[i][j] == sumBack);
        }
        int m = _lattice.lookup_word(i); 
        int n = _lattice.lookup_word(j); 
        }*/
    }

    //cout << "WORDS" << endl;
    for (int i = 0; i < gd.valid_bigrams.size(); i++) {
      Bigram b = gd.valid_bigrams[i];
      {
        GRBLinExpr sumFor, sumBack;
        for (int j = 0; j < gd.forward_bigrams[b.w2].size(); j++) {
          int next = gd.forward_bigrams[b.w2][j];
          sumFor += word_tri_vars[b.w1][b.w2][next];
        }
        
        for (int j = 0; j < gd.backward_bigrams[b.w1].size(); j++) {
          int next = gd.backward_bigrams[b.w1][j];
          sumBack += word_tri_vars[next][b.w1][b.w2];
        }

        if (b.w2 != 0) {
          model->addConstr(word_pair_vars[b.w1][b.w2] == sumFor);
        } else {
          model->addConstr(word_pair_vars[b.w1][b.w2] == 1);
        }
      
        if (b.w1 != _lattice.num_word_nodes - 1) {
          model->addConstr(word_pair_vars[b.w1][b.w2] == sumBack);
        } else {
          model->addConstr(word_pair_vars[b.w1][b.w2] == 1);
        }
      }
    }
  }
  catch (GRBException e) {
    cout << "Error code = " << e.getErrorCode() << endl;
    cout << e.getMessage() << endl;
    return;
  }
  model->update();


}


void LPBuilder::build_all_tri_pairs_lp(Ngram &lm, 
                                       const Cache <LatNode, int> & word_cache,
                                       vector < GRBVar > & word_used_vars,
                                       vector < vector < vector < GRBVar > > > & word_tri_vars,
                                       LatticeVars & lv,
                                       LatticeVars & lv2,
                                       const GraphDecompose & gd) {
  
  vector < vector < GRBVar > >  word_pair_vars;
  
  try{
    lv.initialize_all_pairs(gd, _lattice, model);
    lv2.initialize_all_pairs(gd, _lattice, model);
    initialize_word_pairs(lm, word_cache, gd, word_used_vars, word_pair_vars, word_tri_vars);    
    model->update();
  
    lv.add_all_pairs_constraints(gd, _lattice, model);
    lv2.add_all_pairs_constraints(gd, _lattice, model);

    // bind word_used_vars to _lattice
    /*for (int i = 0; i < _lattice.num_word_nodes; i++) {
      if (!_lattice.is_word(i)) continue;
      
      int phrase_node = _lattice.lookup_word(i);
      model->addConstr(word_used_node[i] == lattice_node_used[phrase_node]);
    } */




    
    // Word constraints    
    //cout << "WORDS" << endl;
    for (int i = 0; i < _lattice.num_word_nodes; i++) {
      
      if (!_lattice.is_word(i)) continue ;      

      {
        int w1 = i;
        GRBLinExpr sumFor;
        bool has = false;
        for (int j = 0; j < gd.forward_bigrams[i].size(); j++) {
          int w2 = gd.forward_bigrams[i][j];
          for (int k = 0; k < gd.forward_bigrams[w2].size(); k++) {
            int w3 = gd.forward_bigrams[w2][k];
            sumFor += word_tri_vars[w1][w2][w3];
            has = true;
          }
        }
        
        if (has) {
          stringstream buf;
          buf << "FOR " << i;
          //cout << "for const" << i<< endl;          
          model->addConstr(word_used_vars[i] == sumFor, buf.str());
        }

      }
        

      {
        int w2 = i;
        GRBLinExpr sumMid;
        bool has = false;
        for (int j = 0; j < gd.forward_bigrams[i].size(); j++) {
          int w3 = gd.forward_bigrams[i][j];
          for (int k = 0; k < gd.backward_bigrams[w2].size(); k++) {
            int w1 = gd.backward_bigrams[w2][k];
            sumMid += word_tri_vars[w1][w2][w3];
            has = true;
          }
        }
        if (has) {
          stringstream buf;
          buf << "MID " << i;
          //cout << "mid const" << i<< endl;
          model->addConstr(word_used_vars[i] == sumMid, buf.str());;
        }
      }


      {
        int w3 = i;
        GRBLinExpr sumBack;
        bool has = false;
        for (int j = 0; j < gd.backward_bigrams[i].size(); j++) {
          int w2 = gd.backward_bigrams[i][j];
          for (int k = 0; k < gd.backward_bigrams[w2].size(); k++) {
            int w1 = gd.backward_bigrams[w2][k];
            //cout << "W! " << w1 << " " << w2 << " " << w3 << " " << has << endl;
            sumBack += word_tri_vars[w1][w2][w3];
            has = true;
          }
        }
        if (has) {
          stringstream buf;
          buf << "BACK " << i;
          //cout << "back const" << i<< " " << has << endl;
          model->addConstr(word_used_vars[i] == sumBack, buf.str());
        }
      }
    }

    vector <vector <GRBLinExpr> >  sums1(_lattice.num_nodes), sums2(_lattice.num_nodes);
    vector <vector <bool> >  has_sums1(_lattice.num_nodes), has_sums2(_lattice.num_nodes);
    

    for (int i = 0; i < _lattice.num_nodes; i++) {
      sums1[i].resize(_lattice.num_nodes);
      sums2[i].resize(_lattice.num_nodes);
      has_sums1[i].resize(_lattice.num_nodes);
      has_sums2[i].resize(_lattice.num_nodes);
      for (int j = 0; j < _lattice.num_nodes; j++) {
        has_sums1[i][j] = false;
        has_sums2[i][j] = false;
      }
    }

    for (unsigned int i=0; i< gd.valid_bigrams.size() ;i++) {
      Bigram b = gd.valid_bigrams[i];
      int w1 = b.w1;
      int w2 = b.w2;
      int phrase_node1 = _lattice.lookup_word(w1);
      int phrase_node2 = _lattice.lookup_word(w2);
      for (int m =0; m < gd.forward_bigrams[w2].size(); m++) {
        int w3 = gd.forward_bigrams[w2][m];
        int phrase_node3 = _lattice.lookup_word(w3);
        
        sums1[phrase_node1][phrase_node2] += word_tri_vars[w1][w2][w3];
        sums2[phrase_node2][phrase_node3] += word_tri_vars[w1][w2][w3];
        has_sums1[phrase_node1][phrase_node2] = true;
        has_sums2[phrase_node2][phrase_node3] = true;
        
        
        //model->addConstr(word_tri_vars[w1][w2][w3] == );
        //model->addConstr(word_tri_vars[w1][w2][w3] == sum);
      }
      
    }


    for (int i = 0; i < _lattice.num_nodes; i++) {
      for (int j = 0; j < _lattice.num_nodes; j++) {
        if (!gd.path_exists(i,j)) continue;
        if ( i==j) continue;
        //cout << "final" << i<< endl;
        if (has_sums1[i][j]) {
          stringstream buf;
          buf << "BILAT  " << i << " " << j ;          
          model->addConstr(sums1[i][j] == lv.all_pairs_exist_vars[i][j], buf.str());
        } 
        if (has_sums2[i][j]) {
          stringstream buf;
          buf << "TRILAT  " << i << " " << j ;          
          model->addConstr(sums2[i][j] == lv2.all_pairs_exist_vars[i][j], buf.str());
        }
        stringstream buf;
        buf << "LATSAME  " << i << " " << j ;          
        
        //model->addConstr(lv.all_pairs_exist_vars[i][j] == 
        //                 lv2.all_pairs_exist_vars[i][j], buf.str());
      }
    }

    /* for (int j = 0; j < gd.backward_bigrams[i].size(); j++) {
          int next = gd.backward_bigrams[i][j];
          sumBack += word_pair_vars[next][i];
        }

        if (i != 0) {
          model->addConstr(word_used_vars[i] == sumFor);
        } else {
          model->addConstr(word_used_vars[i] == 1);
        }
      
        if (i != _lattice.num_word_nodes - 1) {
          model->addConstr(word_used_vars[i] == sumBack);
        } else {
          model->addConstr(word_used_vars[i] == 1);
        }
        }*/

      /*for (int j = 0; j < _lattice.num_word_nodes; j++) {
        if (!_lattice.is_word(j)) continue ;
        
        GRBLinExpr sumFor, sumBack;
        for (int k =0; k < _lattice.num_word_nodes; k++) {
          sumFor += word_tri_vars[i][j][k];
          sumBack += word_tri_vars[k][i][j];
        }
        if (i != 0) {
          model->addConstr(word_pair_vars[i][j] == sumFor);
        }
      
        if (i != _lattice.num_word_nodes - 1) {
          model->addConstr(word_pair_vars[i][j] == sumBack);
        }
        int m = _lattice.lookup_word(i); 
        int n = _lattice.lookup_word(j); 
        }*/
  

    //cout << "WORDS" << endl;
  /*for (int i = 0; i < gd.valid_bigrams.size(); i++) {
      Bigram b = gd.valid_bigrams[i];
      {
        GRBLinExpr sumFor, sumBack;
        for (int j = 0; j < gd.forward_bigrams[b.w2].size(); j++) {
          int next = gd.forward_bigrams[b.w2][j];
          sumFor += word_tri_vars[b.w1][b.w2][next];
        }
        
        for (int j = 0; j < gd.backward_bigrams[b.w1].size(); j++) {
          int next = gd.backward_bigrams[b.w1][j];
          sumBack += word_tri_vars[next][b.w1][b.w2];
        }

        if (b.w2 != 0) {
          model->addConstr(word_pair_vars[b.w1][b.w2] == sumFor);
        } else {
          model->addConstr(word_pair_vars[b.w1][b.w2] == 1);
        }
      
        if (b.w1 != _lattice.num_word_nodes - 1) {
          model->addConstr(word_pair_vars[b.w1][b.w2] == sumBack);
        } else {
          model->addConstr(word_pair_vars[b.w1][b.w2] == 1);
        }
      }
      }*/
  }
  catch (GRBException e) {
    cout << "Error code = " << e.getErrorCode() << endl;
    cout << e.getMessage() << endl;
    return;
  }
  model->update();


}


void LPBuilder::build_hypergraph_lp(vector <GRBVar> & node_vars,  
                                    vector <GRBVar> & edge_vars, 
                                    const Cache<ForestEdge, double> & _weights) {
  try {
    model->set(GRB_StringAttr_ModelName, "Hypergraph");
    //cout << "Variables" << endl;
    for(int i =0; i < _forest.num_nodes(); i++) {
      stringstream buf;
      buf << "NODE" << i;
      node_vars[i] = model->addVar(0.0, 1.0, 0.0 /*Obj*/, VAR_TYPE /*cont*/,  buf.str()/*names*/);
    }

    for (int i=0; i< _forest.num_edges() ; i++ ) { 

      const ForestEdge & edge = _forest.get_edge(i);
      stringstream buf;
      buf << "EDGE" << i;
      //cout << "Edge " << i << " obj " << _weights.get_value(edge) << endl;
      //assert (_weights.has_value(edge)); 
      edge_vars[i] = model->addVar(0.0, 1.0, _weights.get_value(edge) /*Obj*/, VAR_TYPE /*cont*/,  buf.str()/*names*/);
    }
    
    model->update();
    {
      //cout << "ADDING EDGES" << endl;
      for(int i =0; i < _forest.num_nodes(); i++) {
        const ForestNode & node = _forest.get_node(i);

        // Downward edges
        {
          GRBLinExpr sum; 
          for (int j=0; j < node.num_edges(); j++) {
            /* nodes_vars[i] = sum node out ;*/
            sum += edge_vars[node.edge(j).id()];
          }
          if (node.num_edges() > 0) { 
            model->addConstr(node_vars[i] == sum);
          }
        }

        // Upward edges
        { 
          GRBLinExpr sum; 
          for (int j=0; j < node.num_in_edges(); j++) {
            sum += edge_vars[node.in_edge(j).id()];

          }
          if (node.num_in_edges() > 0) { 
            model->addConstr(node_vars[i] == sum);
          }
        }
      }
      
    }
    //cout << "ROOT CONSTRAINT" << endl;
    model->addConstr(node_vars[_forest.root().id()] == 1);
    
    
    model->set(GRB_IntAttr_ModelSense, 1);
    model->update();
  
  
  }
  catch (GRBException e) {
    cout << "Error code = " << e.getErrorCode() << endl;
    cout << e.getMessage() << endl;
    return;
  }  
}


void LPBuilder::solve_hypergraph(const Cache<ForestEdge, double> & _weights) {
  GRBEnv env = GRBEnv();
  model = new GRBModel(env);
  vector <GRBVar> node_vars(_forest.num_nodes());
  vector <GRBVar> edge_vars(_forest.num_edges());
  build_hypergraph_lp( node_vars, edge_vars, _weights);

  model->optimize();

  try {
    for (int i=0; i < _forest.num_nodes(); i++) {
      cout << i << " " << node_vars[i].get(GRB_DoubleAttr_X)<< endl;
      const ForestNode & node = _forest.get_node(i);    
      for (int j=0; j < node.num_edges(); j++) {
        int edge_id = node.edge(j).id();
        cout << "\t" << j << " " << edge_vars[j].get(GRB_DoubleAttr_X) << endl;
      }
    }
  }
  catch (GRBException e) {
    cout << "Error code = " << e.getErrorCode() << endl;
    cout << e.getMessage() << endl;
    return;
  }

}

/*void LPBuilder::solve_full(const Cache<ForestEdge, double> & _weights, const ForestLattice & _lattice, Ngram &lm, const Cache <LatNode, int> & word_cache) {  
  GRBEnv env = GRBEnv();
  GRBModel model = GRBModel(env);
  //vector <GRBVar> node_vars(_forest.num_nodes());
  //vector <GRBVar> edge_vars(_forest.num_edges());
  build_all_pairs_lp(_lattice, model, lm, word_cache);

  // Now add in LM
  }*/


void LPBuilder::solve_full(const Cache<ForestEdge, double> & _weights, 
                           Ngram &lm, 
                           const Cache <LatNode, int> & word_cache) {  
  GraphDecompose gd;
  LatticeVars lv("Bi"),lv2("Tri");

  gd.decompose(&_lattice);

  GRBEnv env = GRBEnv();
  env.set(GRB_StringParam_LogFile, "/tmp/log");
  env.set(GRB_DoubleParam_TimeLimit, 200);
  env.set(GRB_IntParam_OutputFlag, 0);
  model = new GRBModel(env);
  
  //vector <GRBVar> node_vars(_forest.num_nodes());
  //vector <GRBVar> edge_vars(_forest.num_edges());

  vector <GRBVar> node_vars(_forest.num_nodes());
  vector <GRBVar> edge_vars(_forest.num_edges());
  vector < vector < vector < GRBVar > > > word_tri_vars;
  //vector < vector < GRBVar > > all_pairs_exist_vars;
  vector < GRBVar > word_used_vars;

  build_hypergraph_lp(node_vars, edge_vars, _weights);
  //build_all_pairs_lp(lm, word_cache, word_used_vars, word_tri_vars,lv,  gd);
  build_all_tri_pairs_lp(lm, word_cache, word_used_vars, word_tri_vars, lv, lv2, gd);

  // Now add constraints to join the two models
  for (int i =0; i < _forest.num_edges(); i++) {
    vector <int> all = _lattice.original_edges[i];
    for (int j=0; j < all.size();j++) {
      if (_lattice.is_word(all[j])) {
        model->addConstr(word_used_vars[all[j]] == edge_vars[i]);
      } 
    }
  }
  
  for (int i =0; i < _lattice.num_word_nodes; i++) {
    if (_lattice.is_word(i)) continue;
    Bigram b = _lattice.get_nodes_by_labels(i);
    GRBLinExpr sum;
    for (int j=0; j < _lattice.edges_original[i].size(); j++) {
      sum += edge_vars[_lattice.edges_original[i][j]];
    }
    model->addConstr(lv.all_pairs_exist_vars[b.w1][b.w2] == sum);
    model->addConstr(lv2.all_pairs_exist_vars[b.w1][b.w2] == sum);
  }

  // extra trick constraint
  for (int i =0; i < _lattice.num_nodes; i++) {
    if (!_lattice.is_phrase_node(i)) continue;
    for (int j=0; j < _lattice.num_last_bigrams(i); j++) {
      Bigram b = _lattice.last_bigrams(i,j);
      
      for (int w3 = 0; w3 < _lattice.num_word_nodes; w3++) {
        if (!_lattice.is_word(w3)) continue;
        int n1 = _lattice.lookup_word(b.w2);
        int n2 = _lattice.lookup_word(w3);
        if (gd.path_exists(n1, n2)) {
          //model->addConstr(lv.all_pairs_exist_vars[n1][n2] == lv2.all_pairs_exist_vars[n1][n2]);
        }

      }
    }
  }

  // Now add in LM
  model->update();
  model->write("/tmp/graph.lp");
  model->set(GRB_IntAttr_ModelSense, 1);

  model->optimize();
  

  int exact = 0;
  //cout << "WORD UNI" << endl;
  for (int i = 0; i < _lattice.num_word_nodes; i++) {
    if (!_lattice.is_word(i)) continue; 
    double xval = word_used_vars[i].get(GRB_DoubleAttr_X);
    
    //if (xval) {
    //cout << "WORD " << i << xval << endl;
    //}

    if (xval != 0.0 && xval!=1.0) {
      exact +=1;
    }
  }

  for (unsigned int i=0; i< gd.valid_bigrams.size() ;i++) {
    Bigram b = gd.valid_bigrams[i];
    //if (word_pair_vars[b.w1][b.w2].get(GRB_DoubleAttr_X)) {
    //cout << b.w1 << " " << b.w2 << " " << word_pair_vars[b.w1][b.w2].get(GRB_DoubleAttr_X) << " " << word_pair_vars[b.w1][b.w2].get(GRB_DoubleAttr_Obj) << endl;
      
    //}

    for (int j =0; j < gd.forward_bigrams[b.w2].size(); j++) {
      int w3 = gd.forward_bigrams[b.w2][j];
      //cout << b.w1 << " " << b.w2 << " " << w3 << endl;
      double xval =  word_tri_vars[b.w1][b.w2][w3].get(GRB_DoubleAttr_X);

      if (word_tri_vars[b.w1][b.w2][w3].get(GRB_DoubleAttr_X)) {
        //cout << b.w1 << " " << b.w2 << " " << w3 << " " 
        //   << _lattice.lookup_word(b.w1) << " " << _lattice.lookup_word(b.w2) << " " << _lattice.lookup_word(w3) << " " 
        //   << word_tri_vars[b.w1][b.w2][w3].get(GRB_DoubleAttr_X) << " " << word_tri_vars[b.w1][b.w2][w3].get(GRB_DoubleAttr_Obj) << endl;
      
      }
      if ( !(xval == 0.0 || xval == 1.0)) {
        //cerr << b.w1 << " " << b.w2 << " " << w3 << " " << word_tri_vars[b.w1][b.w2][w3].get(GRB_DoubleAttr_X) << " " << word_tri_vars[b.w1][b.w2][w3].get(GRB_DoubleAttr_Obj) << endl;
        //cerr << i << " " << j << " " << all_pairs_exist_vars[i][j].get(GRB_DoubleAttr_X) << " " << all_pairs_exist_vars[i][j].get(GRB_DoubleAttr_Obj) << endl;
        exact +=1;
      }
    }
  }


  for (int i = 0; i < _lattice.num_nodes; i++) {
    for (int j = 0; j < _lattice.num_nodes; j++) {
      if (!gd.path_exists(i,j)) continue; 
      //cout << i << " " << j << endl;
      double x1 =  lv.all_pairs_exist_vars[i][j].get(GRB_DoubleAttr_X);
      double x2 =  lv2.all_pairs_exist_vars[i][j].get(GRB_DoubleAttr_X);
      if (fabs(x1 -x2) >0.001) {
        //cout << "DIFFER " << i << " " << _lattice.is_phrase_node(i) << " " << j << " " <<_lattice.is_phrase_node(j)  <<" "<< x1<< " "<< x2 << endl;
      }
      if (lv.all_pairs_exist_vars[i][j].get(GRB_DoubleAttr_X)) {
        //cout << i << " " << j << " " << all_pairs_exist_vars[i][j].get(GRB_DoubleAttr_X) << " " << all_pairs_exist_vars[i][j].get(GRB_DoubleAttr_Obj) << endl;
      }
      double xval =  lv.all_pairs_exist_vars[i][j].get(GRB_DoubleAttr_X);
      if ( !(xval == 0.0 || xval == 1.0)) {
        //cerr << i << " " << j << " " << all_pairs_exist_vars[i][j].get(GRB_DoubleAttr_X) << " " << all_pairs_exist_vars[i][j].get(GRB_DoubleAttr_Obj) << endl;
        exact += 1;
      }
    }
  }

  /*
  for (int i = 0; i < _lattice.num_nodes; i++) {
    for (int j = 0; j < _lattice.num_nodes; j++) {
      if (!gd.path_exists(i,j)) continue; 
      //cout << i << " " << j << endl;
      if (lv2.all_pairs_exist_vars[i][j].get(GRB_DoubleAttr_X)) {
        //cout << i << " " << j << " " << all_pairs_exist_vars[i][j].get(GRB_DoubleAttr_X) << " " << all_pairs_exist_vars[i][j].get(GRB_DoubleAttr_Obj) << endl;
      }
      double xval =  lv2.all_pairs_exist_vars[i][j].get(GRB_DoubleAttr_X);
      if ( !(xval == 0.0 || xval == 1.0)) {
        //cerr << i << " " << j << " " << all_pairs_exist_vars[i][j].get(GRB_DoubleAttr_X) << " " << all_pairs_exist_vars[i][j].get(GRB_DoubleAttr_Obj) << endl;
        exact += 1;
      }
    }
    }*/

  cout << model->get(GRB_DoubleAttr_ObjVal) << "\t" << model->get(GRB_DoubleAttr_Runtime) << "\t"<<  (exact == 0) << "\t" << exact << endl;
}





const Cache <LatNode, int> * sync_lattice_lm(const ForestLattice  &_lattice, LM & lm) {
  Cache <LatNode, int> *  _cached_words = new Cache <LatNode, int> (_lattice.num_word_nodes);
  int max = lm.vocab.numWords();
  int unk = lm.vocab.getIndex(Vocab_Unknown);
  //assert(false);
  for (int n=0; n < _lattice.num_word_nodes; n++ ) {
    if (!_lattice.is_word(n)) continue;
    
    //const LatNode & node = _lattice.node(n); 
    //assert (node.id() == n);
    string str = _lattice.get_word(n);
    int ind = lm.vocab.getIndex(str.c_str());
    if (ind == -1 || ind > max) { 
      _cached_words->store[n] = unk;
    } else {
      _cached_words->store[n] = ind;
    }
  }
  return _cached_words;
}

int main(int argc, char ** argv) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  
  svector<int, double> * weight;

  {
    // Read the existing address book.
    fstream input(argv[3], ios::in );
    char buf[1000];
    input.getline(buf, 100000);
    string s (buf);
    weight = svector_from_str<int, double>(s);
  }

  
  Vocab * all = new Vocab();
  all->unkIsWord() = true;
  Ngram * lm = new Ngram(*all, 3);

  File file(argv[4], "r", 0);    
  if (!lm->read(file, false)) {
    cerr << "READ FAILURE\n";
  }

  for(int i=1;i<=100;i++) {

      Hypergraph hgraph;
      
      {
        stringstream fname;
        fname <<argv[1] << i;
        fstream input(fname.str().c_str(), ios::in | ios::binary);
        if (!hgraph.ParseFromIstream(&input)) {
          assert (false);
        } 
      }
      
      Forest f (hgraph);
      

      Lattice lat;

      {
        stringstream fname;
        fname <<argv[2] << i;

        fstream input(fname.str().c_str(), ios::in | ios::binary);
        if (!lat.ParseFromIstream(&input)) {
          assert (false);
        }
        
      }

      ForestLattice graph (lat);
      
      
      LPBuilder lp(f, graph);
      
      const Cache <LatNode, int> * word_cache = sync_lattice_lm(graph, *lm); 
      

      Cache<ForestEdge, double> * w = cache_edge_weights(f, *weight);
      cout << i << "\t";

      try {
        lp.solve_full( *w,  *lm, *word_cache);
      } 
      catch (GRBException e) {
        cerr << "Error code = " << e.getErrorCode() << endl;
        cerr << e.getMessage() << endl;
      }
 


      NodeBackCache bcache(f.num_nodes());     
      
      NodeCache ncache(f.num_nodes());
      //double best = best_path(f, *w, ncache, bcache);
      //cout << best << endl;
  }  
  return 1;
}
