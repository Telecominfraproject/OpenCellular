using System.Collections.Generic;
using System.Linq;

namespace cnip.Models
{
    //https://www.geeksforgeeks.org/find-if-there-is-a-path-between-two-vertices-in-a-given-graph/

    // modified for undirected graph

    public static class bfspbvg
    {
        public class Graph
        {
            private readonly int V;    // No. of vertices 
            private readonly List<int>[] adj;    // An array containing adjacency lists 
            public Graph(int V)  // Constructor 
            {
                this.V = V;
                adj = new List<int>[V];
                for (int i = 0; i < V; i++)
                {
                    adj[i] = new List<int>();
                }
            }
            public void AddEdge(int v, int w) // function to add an edge to graph 
            {
                adj[v].Add(w);
                adj[w].Add(v);
            }
            // A BFS based function to check whether d is reachable from s. 
            public bool IsReachable(int s, int d)
            {
                // Base case
                if (s == d)
                {
                    return true;
                }
                // Mark all the vertices as not visited
                bool[] visited = new bool[V];
                for (int i = 0; i < V; i++)
                {
                    visited[i] = false;
                }
                // Create a queue for BFS
                List<int> queue = new List<int>();
                // Mark the current node as visited and enqueue it
                visited[s] = true;
                queue.Add(s);
                // it will be used to get all adjacent vertices of a vertex
                List<int>.Enumerator j;
                while (queue.Count > 0)
                {
                    // Dequeue a vertex from queue
                    s = queue.First();
                    queue.Remove(queue.First());
                    // Get all adjacent vertices of the dequeued vertex s
                    // If a adjacent has not been visited, then mark it visited
                    // and enqueue it
                    for (j = adj[s].GetEnumerator(); j.MoveNext();)
                    {
                        // If this adjacent node is the destination node, then
                        // return true
                        if (j.Current == d)
                        {
                            return true;
                        }
                        // Else, continue to do BFS
                        if (!visited[j.Current])
                        {
                            visited[j.Current] = true;
                            queue.Add(j.Current);
                        }
                    }
                }
                // If BFS is complete without visiting d
                return false;
            }
        }
    }
}
