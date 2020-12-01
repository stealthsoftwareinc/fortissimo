import sys
from Measure import *

party_measures = measureLog(sys.argv[1])

for party in party_measures:
  measures = party_measures[party]
  out = open(party + ".dot", "w")
  out.write("digraph\n")
  out.write("{\n")
  out.write("  graph[ranksep=3.5]")
  for identity in measures:
    measure = measures[identity]
    out.write("  ff" + measure.identity + "[label=\"" + measure.name + "\"];\n")
    if(identity != "0"):
      out.write("  ff" + measure.parent + " -> ff" + measure.identity + ";\n")
  out.write("}")
