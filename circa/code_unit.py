"""
This object contains a single 'unit' of code.

Generally, one unit of code corresponds with one subroutine definition, but the user is
free to do it differently.

All structure-changing operations should be performed on this object,
rather than on the terms themselves.
"""

import terms
from common_errors import UsageError

VERBOSE_DEBUGGING = False

class CodeUnit(object):
  def __init__(self):
    self.main_branch = []
    self.term_namespace = {}
    self.change_listeners = []

  def appendNewTerm(self, function, name=None, inputs=None, branch=None):
    existing_term = terms.findExisting(function,inputs)
    if existing_term: return existing_term

    new_term = terms.Term(function)

    if inputs:
      # If they use any non-term args, convert them to constants
      inputs = map(terms.wrapNonTerm, inputs)
      self.code_unit.setTermInputs(new_term, inputs)

    if branch:
      branch.append(new_term)
    else:
      self.main_branch.append(new_term)

    if name:
      self.setTermName(new_term, name)

  append = appendNewTerm


  def setTermName(self, term, name, allow_rename=False):
    assert isinstance(name, str)

    if (not allow_rename) and (name in self.term_namespace):
      raise UsageError("A term with name \""+str(name)+"\" already exists." +
                       " (Use 'allow_rename' if you want to allow this)")

    self.term_namespace[name] = term

  def setTermInputs(self, target_term, new_inputs):
    "Assigns the term's inputs"

    old_inputs = target_term.inputs
    target_term.inputs = new_inputs

    # if this is a pure function then re-evaluate it
    if target_term.function.pureFunction:
      target_term.evaluate()

    # find which terms were just added
    newly_added = set(new_inputs) - set(old_inputs)

    # find which terms were just removed
    newly_removed = set(old_inputs) - set(new_inputs)

    if not newly_added and not newly_removed:
      return

    # add ourselves to new user lists
    for t in newly_added:
      t.users.add(target_term)

    # remove ourselves from old user lists
    for t in newly_removed:
      t.users.remove(target_term)

  def getNamedTerm(self, name):
    return self.term_namespace[name]

  def evaluate(self):
    if VERBOSE_DEBUGGING: print "code_unit.evaluate"

    for term in self.main_branch:
      if VERBOSE_DEBUGGING: print "Calling evaluate on " + str(term)
      term.evaluate()

  __getitem__ = getNamedTerm

  def iterate(self):
    todo

# Event enumeration
(TERM_APPENDED) = range(1)

class ChangeEvent(object):
  def __init__(self):
    self.changes = []


