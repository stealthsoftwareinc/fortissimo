import re

class Note:
  def __init__(self, t, m, i):
    self.time = t
    self.message = m
    self.index = i
  def itime(self):
    return int(self.time[0:-2])

class Measure:
  def __init__(self, i, n, p, j):
    self.identity = i
    self.name = n
    self.parent = p
    self.notes = list()
    self.index = j
  def addNote(self, note):
    self.notes.append(note)

class Order:
  def __init__(self, t, p, i, pl = 0):
    self.type = t
    self.party = p
    self.identity = i
    self.place = pl

def measureLog(fname, order = list()):
  f = open(fname)
  lines = f.readlines()
  index = 0
  parties = dict()
  for line in lines:
    line_match = re.search("^[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9] [0-9][0-9]:[0-9][0-9]:[0-9][0-9]  TIME \([a-zA-Z0-9 ,{}.-]+\): ", line)
    if line_match is not None:
      party = re.search("\([a-zA-Z0-9 ,{}.-]+\)", line).group()
      party = party[1: -1]
      rest = line[len(line_match.group()):].rstrip()
      if not party in parties:
        parties[party] = dict()
      start_match = re.search("Starting Timer \"FF: ", rest)
      update_match = re.search("Timer \"FF: ", rest)
      if start_match is not None:
        name_match = re.search("FF: .+", rest)
        id_match = re.search("; ID: [0-9]*", rest)
        parent_match = re.search("parent: [0-9]+$|parent: none$", rest)
        name = name_match.group()[len("FF: "): id_match.span()[0] - name_match.span()[0]]
        identity = id_match.group()[len("; ID: "):]
        parent = parent_match.group()[len("parent: "):]
        if identity in parties[party]:
          print("oops: duplicated measure")
        else:
          parties[party][identity] = Measure(identity, name, parent, index)
          order.append(Order("measure", party, identity))
      elif update_match is not None:
        id_match = re.search("; ID: [0-9]*", rest)
        time_match = re.search("; ID: [0-9]*\": [0-9]*us", rest)
        identity = id_match.group()[len("; ID: "):]
        time = time_match.group()[len(id_match.group()) + len("\": "):]
        message = rest[time_match.span()[1] + len(", "):]
        if not identity in parties[party]:
          print("oops: measure not found")
        else:
          parties[party][identity].addNote(Note(time, message, index))
          place = len(parties[party][identity].notes) - 1
          order.append(Order("note", party, identity, place))
      index = index + 1
  return parties
