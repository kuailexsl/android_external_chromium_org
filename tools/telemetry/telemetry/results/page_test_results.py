# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import copy
import logging
import traceback

from telemetry import value as value_module
from telemetry.value import failure

class PageTestResults(object):
  def __init__(self, output_stream=None, trace_tag=''):
    super(PageTestResults, self).__init__()
    self._output_stream = output_stream
    self._trace_tag = trace_tag
    self._current_page = None

    # TODO(chrishenry,eakuefner): Remove self.successes once they can
    # be inferred.
    self.successes = []
    self.skipped = []

    self._representative_value_for_each_value_name = {}
    self._all_page_specific_values = []
    self._all_summary_values = []

  def __copy__(self):
    cls = self.__class__
    result = cls.__new__(cls)
    for k, v in self.__dict__.items():
      if isinstance(v, collections.Container):
        v = copy.copy(v)
      setattr(result, k, v)
    return result

  @property
  def all_page_specific_values(self):
    return self._all_page_specific_values

  @property
  def all_summary_values(self):
    return self._all_summary_values

  @property
  def current_page(self):
    return self._current_page

  @property
  def pages_that_succeeded(self):
    """Returns the set of pages that succeeded."""
    pages = set(value.page for value in self._all_page_specific_values)
    pages.difference_update(self.pages_that_had_failures)
    return pages

  @property
  def pages_that_had_failures(self):
    """Returns the set of failed pages."""
    return set(v.page for v in self.failures)

  @property
  def failures(self):
    values = self._all_page_specific_values
    return [v for v in values if isinstance(v, failure.FailureValue)]

  def _GetStringFromExcInfo(self, err):
    return ''.join(traceback.format_exception(*err))

  def StartTest(self, page):
    self._current_page = page

  def StopTest(self, page):  # pylint: disable=W0613
    self._current_page = None

  def AddValue(self, value):
    self._ValidateValue(value)
    self._all_page_specific_values.append(value)

  def AddSummaryValue(self, value):
    assert value.page is None
    self._ValidateValue(value)
    self._all_summary_values.append(value)

  def _ValidateValue(self, value):
    assert isinstance(value, value_module.Value)
    if value.name not in self._representative_value_for_each_value_name:
      self._representative_value_for_each_value_name[value.name] = value
    representative_value = self._representative_value_for_each_value_name[
        value.name]
    assert value.IsMergableWith(representative_value)

  def AddSkip(self, page, reason):
    self.skipped.append((page, reason))

  def AddSuccess(self, page):
    self.successes.append(page)

  def PrintSummary(self):
    if self.failures:
      logging.error('Failed pages:\n%s', '\n'.join(
          p.display_name for p in self.pages_that_had_failures))

    if self.skipped:
      logging.warning('Skipped pages:\n%s', '\n'.join(
          p.display_name for p in zip(*self.skipped)[0]))

  def FindPageSpecificValuesForPage(self, page, value_name):
    values = []
    for value in self.all_page_specific_values:
      if value.page == page and value.name == value_name:
        values.append(value)
    return values

  def FindAllPageSpecificValuesNamed(self, value_name):
    values = []
    for value in self.all_page_specific_values:
      if value.name == value_name:
        values.append(value)
    return values
