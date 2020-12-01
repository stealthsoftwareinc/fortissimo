import sys
from Measure import *

order = list()
party_measures = measureLog(sys.argv[1], order)

csv = open(sys.argv[1][0:-len(".log")] + "_raw.csv", "w")

csv.write("party, ID, name, time, diff, activity, action\n")

for o in order:
  party = o.party
  identity = o.identity
  measure = party_measures[party][identity]
  name = measure.name
  action = ""
  time = 0
  diff = 0
  activity = "waiting"
  if "measure" == o.type:
    action = "start timer"
  elif "note" == o.type:
    note = measure.notes[o.place]
    time = note.itime()
    prev_time = 0
    if o.place > 0:
      prev_time = measure.notes[o.place - 1].itime()
    diff = time - prev_time
    action = note.message
    if note.message.endswith("end"):
      msg = note.message[0:-len("end")]
      if msg.startswith("handle "):
        msg = msg[len("handle "):]
      if msg.startswith("init"):
        msg = "init"
      elif msg.startswith("receive"):
        msg = "receive"
      elif msg.startswith("complete"):
        msg = "complete"
      elif msg.startswith("promise"):
        msg = "promise"
      activity = msg
  csv.write("\"" + party + "\"")
  csv.write(", ")
  csv.write(identity)
  csv.write(", ")
  csv.write("\"" + name + "\"")
  csv.write(", ")
  csv.write(str(time))
  csv.write(", ")
  csv.write(str(diff))
  csv.write(", ")
  csv.write(activity)
  csv.write(", ")
  csv.write("\"" + action + "\"")
  csv.write("\n")
