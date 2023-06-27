/*               
 * Portions copyright (c) 2003-2007, Paolo Boldi and Sebastiano Vigna. Translation copyright (c) 2007, Jacob Ratkiewicz
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include <sstream>
#include <iterator>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

#include "../webgraph/webgraph.hpp"

int main( int argc, char** argv ) {
   using namespace std;
   using namespace webgraph::bv_graph;
   namespace po = boost::program_options;

   using graph_ptr = graph::graph_ptr;

   std::string name;
   bool directed = false;
   bool remove_degree_zero = false;

   po::options_description options("Options");
   options.add_options()
      ("graph,g", po::value<std::string>(&name)->value_name("<string>")->required(), "Graph basename")
      ("directed,d", po::bool_switch(&directed), "only output directed edges (without inserting the backwards edge)")
      ("remove-degree-zero,r", po::bool_switch(&remove_degree_zero), "remove degree zero nodes");

   po::variables_map cmd_vm;
   po::store(po::parse_command_line(argc, argv, options), cmd_vm);
   po::notify(cmd_vm);

   if (directed && remove_degree_zero) {
      std::cerr << "Error: -d and -r are not compatible." << std::endl;
      std::exit(1);
   }

   graph_ptr gp = graph::load_offline( name );
   
   size_t num_nodes = gp->get_num_nodes();
   size_t num_arcs = gp->get_num_arcs();

   std::vector<std::vector<int>> adjacency_lists(num_nodes);
   std::vector<size_t> index_mapping(num_nodes);
   auto check_is_in_range = [&](const size_t index) {
      if (index > adjacency_lists.size()) {
         std::cerr << "Error: size mismatch." << std::endl;
         std::exit(1);
      }
   };

   auto [n, n_end] = gp->get_node_iterator( 0 );
   size_t i = 0;
   size_t undirected_edges = 0;
   while( n != n_end ) {
      check_is_in_range(i);

      std::vector<int> outgoing = successor_vector( n );
      std::sort(outgoing.begin(), outgoing.end());
      if (directed) {
         adjacency_lists[i] = std::move(outgoing);
      } else {
         // important note: adjacency_lists[i] is currently sorted
         // due to the way we are constructing the lists
         size_t j = 0;
         for (int t: outgoing) {
            size_t target = static_cast<size_t>(t);
            if (target > i) {
               check_is_in_range(target);
               adjacency_lists[i].push_back(target);
               adjacency_lists[target].push_back(i);
               undirected_edges++;
            } else if (target < i) {
               // check whether the edge was already inserted
               while (j < adjacency_lists[i].size() && adjacency_lists[i][j] < t) {
                  ++j;
               }
               if (j == adjacency_lists[i].size() || adjacency_lists[i][j] != t) {
                  // edge not inserted yet
                  adjacency_lists[i].push_back(target);
                  adjacency_lists[target].push_back(i);
                  undirected_edges++;
               }
            }
         }
      }
      ++i;
      ++n;  
   }

   if (remove_degree_zero) {
      size_t new_index = 0;
      for (size_t i = 0; i < adjacency_lists.size(); ++i) {
         index_mapping[i] = new_index;
         if (adjacency_lists[i].size() > 0) {
            new_index++;
         }
      }
      num_nodes = new_index;
   }

   std::cout << num_nodes << " " << (directed ? num_arcs : undirected_edges) << std::endl;
   for (const auto& outgoing: adjacency_lists) {
      for (int t: outgoing) {
         size_t target = static_cast<size_t>(t);
         if (remove_degree_zero) {
            target = index_mapping[target];
         }
         // in metis format, indices start with 1
         std::cout << (target + 1) << " ";
      }
      if (!remove_degree_zero || outgoing.size() > 0) {
         std::cout << std::endl;
      }
   }

   return 0;  
}
