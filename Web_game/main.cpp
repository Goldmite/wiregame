#include <iostream>
#include <map>
#include <vector>
#include <unordered_set>
#include <sstream>
#include <emscripten.h>

using namespace std;

int CENTERLINE = 7;
#define LENGTH 10
#define UPPERBOUND 14
#define LOWERBOUND 0
class DirectedGraph {
private:
  // Node structure to store metadata
  struct Node {
      int value;                            // Node's value
      unsigned char color;                  // Color for score when matching
      unsigned char continuation;           // Color continuation aka streak of same colored lines
      vector<pair<int, int>> children;      // List of child coords (acts like IDs aswell)
      vector<pair<int, int>> parents;       // List of parent coords
  };
  bool hitBounds = false;
  int NodesInCol[LENGTH] = {0}; //up to 10 col while testing
  map<pair<int, int>, Node> nodes; // Adjacency list storing nodes by ID - position
  pair<int, int> pos; //bounds between y = -10 & y = 10
  map<int, unordered_set<int>> columnSets; //keep track of used numbers in a column
public:
  DirectedGraph() {
    pos = {0, CENTERLINE}; //Starting node initialization
    nodes[pos];
    NodesInCol[0] = 1;
  }
  // Add a new node
  void createNode(int branches, bool aboveCenter) {
      int& x = pos.first;
      int& y = pos.second;
      pair<int, int> PARENT = {x, y};
      int belowMain = branches / 2;
      if (aboveCenter && branches % 2 == 0) belowMain--;//inverts the even number branching direction from down to up because aboveMain increases from this
      for (int h = y + 1; h <= y + belowMain; h++) //below node
      {
        addEdge(PARENT, {x + 1, h});
      }
      int aboveMain = branches - belowMain - 1; //main and above node
      for (int h = y; h >= y - aboveMain; h--)
      {
        addEdge(PARENT, {x + 1, h});
      }
      //adjust centerline to starting point
      if (x == 0) {
        CENTERLINE = y;
      }
      return;
  }
  // Add a directed edge from parent to child
  void addEdge(pair<int, int> parent, pair<int, int> child) {
      if (child.second > UPPERBOUND || child.second < LOWERBOUND)
      {
        hitBounds = true;
        return;
      }
      if (nodes.find(child) == nodes.end()) NodesInCol[child.first]++;

      nodes[parent].children.push_back(child); //parent add child
      nodes[child].parents.push_back(parent); //child add parent
      return;
  }

  void addValue(int value, int x, int y) {
    //pair<int, int> coords = {x, y};
    pos = {x, y};
    if (columnSets[pos.first].find(value) != columnSets[pos.first].end() && value != 0) //is used
    {
      cout << "Can't use number " << value << " as it is not unique to column " << pos.first << endl;
      return;
    }
    nodes[pos].value = value;
    columnSets[pos.first].insert(value);
    if (value == 0) return;
    nodes[pos].continuation = 0;
    createNode(value, pos.second <= CENTERLINE);
    return;
  }
  void setColor(int x, int y, unsigned char c)
  {
    nodes[{x, y}].color = c;
  }
  unsigned char getColor(int x, int y)
  {
    return nodes[{x, y}].color;
  }
  unsigned char updateColorStreak(int px, int py, int cx, int cy)
  {
    nodes[{cx, cy}].continuation = nodes[{px, py}].continuation + 1;
    return nodes[{cx, cy}].continuation;
  }
  int sizeOfCol(int x)
  {
    return NodesInCol[x];
  }
  bool getState()
  {
    return hitBounds;
  }
  bool isParentColFilled(int parentCol) //function called on child creation
  {
    //check columnSets[parentCol].size() against the amount of nodes in parent column.
    return columnSets[parentCol].size() == NodesInCol[parentCol];
  }
  
  bool ChildHaveParent(int cx, int cy)
  {
    //pair<int, int> cp = {cx, cy};
    auto key = nodes.find({cx, cy});
    if (key != nodes.end() && key->second.parents.size() > 0)
    {
      return true;
    }
    return false;
  }
  int getYofnthChild(int px, int py, int n)
  {
    const Node& parent = nodes[{px, py}];
    if (parent.children.size() <= n) return -1; //child out of bounds, return invalid y level
    return parent.children[n].second; //return parent's nth child y-level
  }
  int getYofnthParent(int cx, int cy, int n)
  {
    return nodes[{cx, cy}].parents[n].second; //return child's nth parent y-level
  }
  int getNumberofParents(int cx, int cy)
  {
    return nodes[{cx, cy}].parents.size();
  }
  // Display the adjacency list
  void displayGraph() {
    int out[UPPERBOUND][LENGTH];
    for (int i = 0; i <= UPPERBOUND; i++)
    {
      for (int j = 0; j < LENGTH; j++)
      {
        out[i][j] = -1;
      } 
    }
    /*
    for (const auto& pair : nodes) {
      const auto& nodeId = pair.first;
      const Node& node = pair.second;
      cout <<"COORDS: " << "y: " << nodeId.second << " x: " << nodeId.first << " value: " << node.value << endl;
      out[nodeId.second][nodeId.first] = node.value;
      if (node.value == 0) continue;
      cout << "NID " << nodeId.first << " " << nodeId.second << " V: " << node.value;
      cout << " CIDs -> [";
      for (auto i : node.children) {
          cout << i.first << " " << i.second << ", ";
      }
      cout << "], PIDs -> [";
      for (auto i : node.parents) {
          cout << i.first << " " << i.second << ", ";
      }
      cout << "]" << endl;
    }
    for (int i = 0; i < UPPERBOUND; i++)
    {
      for (int j = 0; j < 5; j++)
      {
        if (out[i][j] == -1) cout << ".";
        else cout << out[i][j];
      }
      cout << endl;
    }
  }*/
 // Prepare output string for JavaScript
  stringstream graphOutput;
  
  for (const auto& pair : nodes) {
      const auto& nodeId = pair.first;
      const Node& node = pair.second;

      // Adding node info to output
      graphOutput << "COORDS: y: " << nodeId.second << " x: " << nodeId.first << " value: " << node.value << "\n";
      out[nodeId.second][nodeId.first] = node.value;

      if (node.value == 0) continue;

      graphOutput << "NID " << nodeId.first << " " << nodeId.second << " V: " << node.value;
      graphOutput << " CIDs -> [";
      
      for (auto i : node.children) {
          graphOutput << i.first << " " << i.second << ", ";
      }

      graphOutput << "], PIDs -> [";
      
      for (auto i : node.parents) {
          graphOutput << i.first << " " << i.second << ", ";
      }

      graphOutput << "]\n";
  }

  // Add a new line before the graph visualization
  graphOutput << "\nGraph Visualization:\n";

  for (int i = 0; i <= UPPERBOUND; i++) {
      for (int j = 0; j < LENGTH; j++) {
          if (out[i][j] == -1) {
              graphOutput << ".";
          } else {
              graphOutput << out[i][j];
          }
      }
      graphOutput << "\n";
  }
  
  // Send the graph data to the webpage using EM_ASM
  /*EM_ASM({
      var outputElement = document.getElementById("output");
      var msg = UTF8ToString($0);
      outputElement.innerHTML += "<pre>" + msg + "</pre>";
  }, graphOutput.str().c_str());*/
}
  

};

int main() {
    //DirectedGraph graph;
/*
    
    update command: 
    emcc main.cpp -o nodetest.js -s NO_EXIT_RUNTIME=1 -s EXPORTED_RUNTIME_METHODS=ccall,cwrap,FS
*/

    /*
    while (graph.getEmptyNodes().size() != 0 && !graph.getState())
    {
      int v;
      auto e = graph.getEmptyNodes();
      for (auto p : e)
      {
        
        cout << "Enter value [1-9] for node("<<p.first.first << ","<<p.first.second << "):" << endl;
        
        cin >> v;
        if (v <= LOWERBOUND || v >= (UPPERBOUND)) break;
        graph.addValue(v, p.first.first, p.first.second);
        if (graph.getState()) break;
        if (p.first.first == 4 ) //if at col 4 and entered value = reached col 5
        {
          cout << "You reached the finish line!" << endl;
          v = 0;
          break;
        }
      }
      if (v <= LOWERBOUND || v >= (UPPERBOUND)) break;
    }
    if(graph.getState()) cout << "You lose! Exceeded bounds ["<<LOWERBOUND<<","<<UPPERBOUND<<"]"<<endl;
    // Display the graph
    graph.displayGraph();
    */
    return 0;
}

DirectedGraph* globalNet = nullptr;
extern "C"
{
  EMSCRIPTEN_KEEPALIVE
  void createGraph() {
    globalNet = new DirectedGraph(); 
  }
  EMSCRIPTEN_KEEPALIVE
  void createNode(int branches, bool aboveCenter) {
    if (globalNet) globalNet->createNode(branches, aboveCenter);
  }
  EMSCRIPTEN_KEEPALIVE
  void addEdge(pair<int, int> parent, pair<int, int> child) {
    if (globalNet) globalNet->addEdge(parent, child);
  }
  EMSCRIPTEN_KEEPALIVE
  void addValue(int value, int x, int y) {
    globalNet->addValue(value, x, y);
  }
  EMSCRIPTEN_KEEPALIVE
  void setColor(int x, int y, unsigned char c) {
    globalNet->setColor(x, y, c);
  }
  EMSCRIPTEN_KEEPALIVE
  unsigned char getColor(int x, int y) {
    return globalNet->getColor(x, y);
  }
  EMSCRIPTEN_KEEPALIVE
  unsigned char updateColorStreak(int px, int py, int cx, int cy)
  {
    return globalNet->updateColorStreak(px, py, cx, cy);
  }
  EMSCRIPTEN_KEEPALIVE
  void displayGraph() {
    if (globalNet) globalNet->displayGraph();
  }
  EMSCRIPTEN_KEEPALIVE
  int sizeOfCol(int x) {
    return globalNet->sizeOfCol(x);
  }
  EMSCRIPTEN_KEEPALIVE
  bool ChildHaveParent(int cx, int cy) {
    return globalNet->ChildHaveParent(cx, cy);
  }
  EMSCRIPTEN_KEEPALIVE
  bool isParentColFilled(int parentCol) {
    return globalNet->isParentColFilled(parentCol);
  }
  EMSCRIPTEN_KEEPALIVE
  bool getState() {
    return globalNet->getState();
  }
  EMSCRIPTEN_KEEPALIVE
  int getYofnthChild(int px, int py, int n) {
    return globalNet->getYofnthChild(px, py, n);
  }
  EMSCRIPTEN_KEEPALIVE
  int getYofnthParent(int cx, int cy, int n) {
    return globalNet->getYofnthParent(cx, cy, n);
  }
  EMSCRIPTEN_KEEPALIVE
  int getNumberofParents(int cx, int cy) {
    return globalNet->getNumberofParents(cx, cy);
  }
}