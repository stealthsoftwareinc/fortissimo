import sys
from Measure import *

def median(lst):
  if len(lst) == 0:
    return 0
  lst.sort()
  idx = int((len(lst) - 1) / 2)
  if idx % 2 == 0 and idx > 1:
    return (lst[idx] + lst[idx + 1]) / 2
  else:
    return lst[idx]

def minimum(lst):
  if len(lst) == 0:
    return 0
  lst.sort()
  return lst[0]

def maximum(lst):
  if len(lst) == 0:
    return 0
  lst.sort()
  return lst[len(lst) - 1]

def mean(lst):
  if len(lst) > 0:
    return sum(lst) / len(lst)
  else:
    return 0

def variance(lst):
  if len(lst) == 0:
    return 0
  u = mean(lst)
  v = 0
  for x in lst:
    v = v + (x - u) * (x - u)
  return v / len(lst)

def has_differences(lst):
  if len(lst) == 0:
    return False
  first = 0
  second = False
  for x in lst:
    if not second:
      first = x
      second = True
    elif x != first:
      return True
  return False

def batchTotal(name):
  if name.startswith("Batch size: "):
    name = name[len("Batch size: "):]
    count_str = re.match("[0-9]+", name).group()
    count = int(count_str)
    return count * batchTotal(name[len(count_str) + len(" of "):])
  else:
    return 1

def batchDepth(name):
  match = re.match("Batch size: [0-9]+ of ", name)
  if match is not None:
    return 1 + batchDepth(name[len(match.group()):])
  else:
    return 0

def batchType(name):
  match = re.match("Batch size: [0-9]+ of ", name)
  if match is not None:
    return batchType(name[len(match.group()):])
  else:
    return name

class Fronctocol:
  def __init__(self):
    self.count = 0
    self.totals = list()
    self.actives = list()
    self.waitings = list()
    self.inits = list()
    self.receives = list()
    self.completes = list()
    self.promises = list()
    self.meta_actives = list()
    self.meta_waitings = list()
    self.meta_receives = list()
    self.meta_completes = list()
    self.meta_promises = list()
    self.init_times = list()

party_measures = measureLog(sys.argv[1])
fronctocols = dict()

indiv = open(sys.argv[1][0:-len(".log")] + "_individual.csv", "w")
indiv.write("Party, fronctocol, ID, batch_depth, batch_total, batch_type, total_time, init_place, init_delay, free_place, free_delay, init_time, " \
  + "active_count, active_total, active_min, active_max, active_mean, active_variance, active_median, " \
  + "waiting_count, waiting_total, waiting_min, waiting_max, waiting_mean, waiting_variance, waiting_median, " \
  + "receive_count, receive_total, receive_min, receive_max, receive_mean, receive_variance, receive_median, " \
  + "complete_count, complete_total, complete_min, complete_max, complete_mean, complete_variance, complete_median, " \
  + "promise_count, promise_total, promise_min, promise_max, promise_mean, promise_variance, promise_median" \
  + "\n")

def indiv_write(measure, party, f):
  f.write("\"" + party + "\"")
  f.write(", ")
  f.write("\"" + measure.name + "\"")
  f.write(", ")
  f.write(measure.identity)
  f.write(", ")
  f.write(str(batchDepth(measure.name)))
  f.write(", ")
  f.write(str(batchTotal(measure.name)))
  f.write(", ")
  f.write("\"" + batchType(measure.name) + "\"")
  f.write(", ")
  f.write(str(measure.running_time))
  f.write(", ")
  f.write(measure.init_place)
  f.write(", ")
  f.write(str(measure.init_delay))
  f.write(", ")
  f.write(measure.free_place)
  f.write(", ")
  f.write(str(measure.free_delay))
  f.write(", ")
  f.write(str(measure.init_time))
  f.write(", ")
  f.write(str(len(measure.actives)))
  f.write(", ")
  f.write(str(sum(measure.actives)))
  f.write(", ")
  f.write(str(minimum(measure.actives)))
  f.write(", ")
  f.write(str(maximum(measure.actives)))
  f.write(", ")
  f.write(str(mean(measure.actives)))
  f.write(", ")
  f.write(str(variance(measure.actives)))
  f.write(", ")
  f.write(str(median(measure.actives)))
  f.write(", ")
  f.write(str(len(measure.waitings)))
  f.write(", ")
  f.write(str(sum(measure.waitings)))
  f.write(", ")
  f.write(str(minimum(measure.waitings)))
  f.write(", ")
  f.write(str(maximum(measure.waitings)))
  f.write(", ")
  f.write(str(mean(measure.waitings)))
  f.write(", ")
  f.write(str(variance(measure.waitings)))
  f.write(", ")
  f.write(str(median(measure.waitings)))
  f.write(", ")
  f.write(str(len(measure.receives)))
  f.write(", ")
  f.write(str(sum(measure.receives)))
  f.write(", ")
  f.write(str(minimum(measure.receives)))
  f.write(", ")
  f.write(str(maximum(measure.receives)))
  f.write(", ")
  f.write(str(mean(measure.receives)))
  f.write(", ")
  f.write(str(variance(measure.receives)))
  f.write(", ")
  f.write(str(median(measure.receives)))
  f.write(", ")
  f.write(str(len(measure.completes)))
  f.write(", ")
  f.write(str(sum(measure.completes)))
  f.write(", ")
  f.write(str(minimum(measure.completes)))
  f.write(", ")
  f.write(str(maximum(measure.completes)))
  f.write(", ")
  f.write(str(mean(measure.completes)))
  f.write(", ")
  f.write(str(variance(measure.completes)))
  f.write(", ")
  f.write(str(median(measure.completes)))
  f.write(", ")
  f.write(str(len(measure.promises)))
  f.write(", ")
  f.write(str(sum(measure.promises)))
  f.write(", ")
  f.write(str(minimum(measure.promises)))
  f.write(", ")
  f.write(str(maximum(measure.promises)))
  f.write(", ")
  f.write(str(mean(measure.promises)))
  f.write(", ")
  f.write(str(variance(measure.promises)))
  f.write(", ")
  f.write(str(median(measure.promises)))
  f.write("\n")

for party in party_measures:
  measures = party_measures[party]
  for identity in measures:
    measure = measures[identity]
    free_note = measure.notes[len(measure.notes) - 1]
    complete_note = measure.notes[len(measure.notes) - 2]
    if not free_note.message.startswith("freed"):
      print("oops: expected freed message " + free_note.message)
    if not complete_note.message.startswith("complete"):
      print("oops: expected complete message " + complete_note.message)
    if measure.identity == "0":
      if not measure.notes[0].message.startswith("main init"):
        print("oops: main init message is wrong")
    elif not measure.notes[0].message.startswith("init"):
      print("oops: init message is wrong " + measure.notes[0].message)
    if not measure.notes[1].message.endswith("end"):
      print("oops: init end message is wrong")
    measure.free_delay = free_note.itime() - complete_note.itime()
    measure.free_place = free_note.message[len("freed "):]
    measure.init_delay = measure.notes[0].itime()
    measure.init_place = measure.notes[0].message[len("init"): -len("start")]
    measure.init_time = measure.notes[1].itime() - measure.notes[0].itime()
    measure.running_time = complete_note.itime() - measure.init_delay
    prev_end = measure.notes[1].itime()
    i = 2
    measure.receives = list()
    measure.completes = list()
    measure.promises = list()
    measure.waitings = list()
    measure.actives = list()
    measure.actives.append(measure.init_time)
    while i < len(measure.notes) - 2:
      start = measure.notes[i]
      if start.message.endswith("start"):
        end = measure.notes[i + 1]
        if not end.message.endswith("end"):
          print("oops: next line does not end: " + end)
        i = i + 1
        diff = end.itime() - start.itime()
        wait = start.itime() - prev_end
        measure.waitings.append(wait)
        measure.actives.append(diff)
        prev_end = end.itime()
        if start.message.startswith("handle receive"):
          measure.receives.append(diff)
        elif start.message.startswith("handle complete"):
          measure.completes.append(diff)
        elif start.message.startswith("handle promise"):
          measure.promises.append(diff)
        else:
          print("oops: unexpected line: " + start.message)
      else:
        print("oops: unexpected line: " + start.message)
      i = i + 1
    if not measure.name in fronctocols:
      fronctocols[measure.name] = Fronctocol()
    fronctocols[measure.name].totals.append(measure.running_time)
    fronctocols[measure.name].inits.append(measure.init_time)
    fronctocols[measure.name].receives.extend(measure.receives)
    fronctocols[measure.name].meta_receives.append(len(measure.receives))
    fronctocols[measure.name].completes.extend(measure.completes)
    fronctocols[measure.name].meta_completes.append(len(measure.completes))
    fronctocols[measure.name].promises.extend(measure.promises)
    fronctocols[measure.name].meta_promises.append(len(measure.promises))
    fronctocols[measure.name].waitings.extend(measure.waitings)
    fronctocols[measure.name].meta_waitings.append(len(measure.waitings))
    fronctocols[measure.name].actives.extend(measure.actives)
    fronctocols[measure.name].meta_actives.append(len(measure.actives))
    fronctocols[measure.name].count = fronctocols[measure.name].count + 1
    indiv_write(measure, party, indiv)

bn = open(sys.argv[1][0:-len(".log")] + "_by_name.csv", "w")
bn.write("name, batch_depth, batch_total, batch_type, count, " \
  + "running_total, running_min, running_max, running_mean, running_variance, running_median, " \
  + "active_count_per_median, active_count_per_has_differences, active_count, active_total, active_min, active_max, active_mean, active_variance, active_median, " \
  + "waiting_count_per_median, waiting_count_per_has_differences, waiting_count, waiting_total, waiting_min, waiting_max, waiting_mean, waiting_variance, waiting_median, " \
  + "init_total, init_min, init_max, init_mean, init_variance, init_median, " \
  + "receive_count_per_median, receive_count_per_has_differences, receive_count, receive_total, receive_min, receive_max, receive_mean, receive_variance, receive_median, " \
  + "complete_count_per_median, complete_count_per_has_differences, complete_count, complete_total, complete_min, complete_max, complete_mean, complete_variance, complete_median, " \
  + "promise_count_per_median, promise_count_per_has_differences, promise_count, promise_total, promise_min, promise_max, promise_mean, promise_variance, promise_median" \
  + "\n")

for name in fronctocols:
  fronctocol = fronctocols[name]
  bn.write("\"" + name + "\"")
  bn.write(", ")
  bn.write(str(batchDepth(name)))
  bn.write(", ")
  bn.write(str(batchTotal(name)))
  bn.write(", ")
  bn.write("\"" + batchType(name) + "\"")
  bn.write(", ")
  bn.write(str(fronctocol.count))
  bn.write(", ")
  bn.write(str(sum(fronctocol.totals)))
  bn.write(", ")
  bn.write(str(minimum(fronctocol.totals)))
  bn.write(", ")
  bn.write(str(maximum(fronctocol.totals)))
  bn.write(", ")
  bn.write(str(mean(fronctocol.totals)))
  bn.write(", ")
  bn.write(str(variance(fronctocol.totals)))
  bn.write(", ")
  bn.write(str(median(fronctocol.totals)))
  bn.write(", ")
  bn.write(str(median(fronctocol.meta_actives)))
  bn.write(", ")
  bn.write(str(has_differences(fronctocol.meta_actives)))
  bn.write(", ")
  bn.write(str(len(fronctocol.actives)))
  bn.write(", ")
  bn.write(str(sum(fronctocol.actives)))
  bn.write(", ")
  bn.write(str(minimum(fronctocol.actives)))
  bn.write(", ")
  bn.write(str(maximum(fronctocol.actives)))
  bn.write(", ")
  bn.write(str(mean(fronctocol.actives)))
  bn.write(", ")
  bn.write(str(variance(fronctocol.actives)))
  bn.write(", ")
  bn.write(str(median(fronctocol.actives)))
  bn.write(", ")
  bn.write(str(median(fronctocol.meta_waitings)))
  bn.write(", ")
  bn.write(str(has_differences(fronctocol.meta_waitings)))
  bn.write(", ")
  bn.write(str(len(fronctocol.waitings)))
  bn.write(", ")
  bn.write(str(sum(fronctocol.waitings)))
  bn.write(", ")
  bn.write(str(minimum(fronctocol.waitings)))
  bn.write(", ")
  bn.write(str(maximum(fronctocol.waitings)))
  bn.write(", ")
  bn.write(str(mean(fronctocol.waitings)))
  bn.write(", ")
  bn.write(str(variance(fronctocol.waitings)))
  bn.write(", ")
  bn.write(str(median(fronctocol.waitings)))
  bn.write(", ")
  bn.write(str(sum(fronctocol.inits)))
  bn.write(", ")
  bn.write(str(minimum(fronctocol.inits)))
  bn.write(", ")
  bn.write(str(maximum(fronctocol.inits)))
  bn.write(", ")
  bn.write(str(mean(fronctocol.inits)))
  bn.write(", ")
  bn.write(str(variance(fronctocol.inits)))
  bn.write(", ")
  bn.write(str(median(fronctocol.inits)))
  bn.write(", ")
  bn.write(str(median(fronctocol.meta_receives)))
  bn.write(", ")
  bn.write(str(has_differences(fronctocol.meta_receives)))
  bn.write(", ")
  bn.write(str(len(fronctocol.receives)))
  bn.write(", ")
  bn.write(str(sum(fronctocol.receives)))
  bn.write(", ")
  bn.write(str(minimum(fronctocol.receives)))
  bn.write(", ")
  bn.write(str(maximum(fronctocol.receives)))
  bn.write(", ")
  bn.write(str(mean(fronctocol.receives)))
  bn.write(", ")
  bn.write(str(variance(fronctocol.receives)))
  bn.write(", ")
  bn.write(str(median(fronctocol.receives)))
  bn.write(", ")
  bn.write(str(median(fronctocol.meta_completes)))
  bn.write(", ")
  bn.write(str(has_differences(fronctocol.meta_completes)))
  bn.write(", ")
  bn.write(str(len(fronctocol.completes)))
  bn.write(", ")
  bn.write(str(sum(fronctocol.completes)))
  bn.write(", ")
  bn.write(str(minimum(fronctocol.completes)))
  bn.write(", ")
  bn.write(str(maximum(fronctocol.completes)))
  bn.write(", ")
  bn.write(str(mean(fronctocol.completes)))
  bn.write(", ")
  bn.write(str(variance(fronctocol.completes)))
  bn.write(", ")
  bn.write(str(median(fronctocol.completes)))
  bn.write(", ")
  bn.write(str(median(fronctocol.meta_promises)))
  bn.write(", ")
  bn.write(str(has_differences(fronctocol.meta_promises)))
  bn.write(", ")
  bn.write(str(len(fronctocol.promises)))
  bn.write(", ")
  bn.write(str(sum(fronctocol.promises)))
  bn.write(", ")
  bn.write(str(minimum(fronctocol.promises)))
  bn.write(", ")
  bn.write(str(maximum(fronctocol.promises)))
  bn.write(", ")
  bn.write(str(mean(fronctocol.promises)))
  bn.write(", ")
  bn.write(str(variance(fronctocol.promises)))
  bn.write(", ")
  bn.write(str(median(fronctocol.promises)))
  bn.write("\n")
