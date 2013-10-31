#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from copy import deepcopy
import json
import os
import sys
import unittest

from api_data_source import (_JSCModel,
                             _FormatValue,
                             _GetAddRulesDefinitionFromEvents)
from branch_utility import ChannelInfo
from collections import namedtuple
from compiled_file_system import CompiledFileSystem
from file_system import FileNotFoundError
from future import Future
from object_store_creator import ObjectStoreCreator
from reference_resolver import ReferenceResolver
from test_branch_utility import TestBranchUtility
from test_data.canned_data import CANNED_TEST_FILE_SYSTEM_DATA
from test_file_system import TestFileSystem
import third_party.json_schema_compiler.json_parse as json_parse


def _MakeLink(href, text):
  return '<a href="%s">%s</a>' % (href, text)


def _GetType(dict_, name):
  for type_ in dict_['types']:
    if type_['name'] == name:
      return type_


class FakeAvailabilityFinder(object):

  def GetApiAvailability(self, version):
    return ChannelInfo('stable', '396', 5)


class FakeSamplesDataSource(object):

  def Create(self, request):
    return {}


class FakeAPIAndListDataSource(object):

  def __init__(self, json_data):
    self._json = json_data

  def Create(self, *args, **kwargs):
    return self

  def get(self, key, disable_refs=False):
    if key not in self._json:
      raise FileNotFoundError(key)
    return self._json[key]

  def GetAllNames(self):
    return self._json.keys()


class FakeTemplateCache(object):

  def GetFromFile(self, key):
    return Future(value='handlebar %s' % key)


class APIDataSourceTest(unittest.TestCase):

  def setUp(self):
    self._base_path = os.path.join(sys.path[0], 'test_data', 'test_json')
    self._compiled_fs_factory = CompiledFileSystem.Factory(
        ObjectStoreCreator.ForTest())
    self._json_cache = self._compiled_fs_factory.Create(
        TestFileSystem(CANNED_TEST_FILE_SYSTEM_DATA),
        lambda _, json: json_parse.Parse(json),
        APIDataSourceTest,
        'test')

  def _ReadLocalFile(self, filename):
    with open(os.path.join(self._base_path, filename), 'r') as f:
      return f.read()

  def _CreateRefResolver(self, filename):
    data_source = FakeAPIAndListDataSource(
        self._LoadJSON(filename))
    return ReferenceResolver.Factory(data_source,
                                     data_source,
                                     ObjectStoreCreator.ForTest()).Create()

  def _LoadJSON(self, filename):
    return json.loads(self._ReadLocalFile(filename))

  def testCreateId(self):
    data_source = FakeAPIAndListDataSource(
        self._LoadJSON('test_file_data_source.json'))
    dict_ = _JSCModel(self._LoadJSON('test_file.json')[0],
                      self._CreateRefResolver('test_file_data_source.json'),
                      False,
                      FakeAvailabilityFinder(),
                      TestBranchUtility.CreateWithCannedData(),
                      self._json_cache,
                      FakeTemplateCache(),
                      None).ToDict()
    self.assertEquals('type-TypeA', dict_['types'][0]['id'])
    self.assertEquals('property-TypeA-b',
                      dict_['types'][0]['properties'][0]['id'])
    self.assertEquals('method-get', dict_['functions'][0]['id'])
    self.assertEquals('event-EventA', dict_['events'][0]['id'])

  # TODO(kalman): re-enable this when we have a rebase option.
  def DISABLED_testToDict(self):
    filename = 'test_file.json'
    expected_json = self._LoadJSON('expected_' + filename)
    data_source = FakeAPIAndListDataSource(
        self._LoadJSON('test_file_data_source.json'))
    dict_ = _JSCModel(self._LoadJSON(filename)[0],
                      self._CreateRefResolver('test_file_data_source.json'),
                      False,
                      FakeAvailabilityFinder(),
                      TestBranchUtility.CreateWithCannedData(),
                      self._json_cache,
                      FakeTemplateCache(),
                      None).ToDict()
    self.assertEquals(expected_json, dict_)

  def testFormatValue(self):
    self.assertEquals('1,234,567', _FormatValue(1234567))
    self.assertEquals('67', _FormatValue(67))
    self.assertEquals('234,567', _FormatValue(234567))

  def testFormatDescription(self):
    dict_ = _JSCModel(self._LoadJSON('ref_test.json')[0],
                      self._CreateRefResolver('ref_test_data_source.json'),
                      False,
                      FakeAvailabilityFinder(),
                      TestBranchUtility.CreateWithCannedData(),
                      self._json_cache,
                      FakeTemplateCache(),
                      None).ToDict()
    self.assertEquals(_MakeLink('ref_test.html#type-type2', 'type2'),
                      _GetType(dict_, 'type1')['description'])
    self.assertEquals(
        'A %s, or %s' % (_MakeLink('ref_test.html#type-type3', 'type3'),
                         _MakeLink('ref_test.html#type-type2', 'type2')),
        _GetType(dict_, 'type2')['description'])
    self.assertEquals(
        '%s != %s' % (_MakeLink('other.html#type-type2', 'other.type2'),
                      _MakeLink('ref_test.html#type-type2', 'type2')),
        _GetType(dict_, 'type3')['description'])


  def testGetApiAvailability(self):
    model = _JSCModel(self._LoadJSON('test_file.json')[0],
                      self._CreateRefResolver('test_file_data_source.json'),
                      False,
                      FakeAvailabilityFinder(),
                      TestBranchUtility.CreateWithCannedData(),
                      self._json_cache,
                      FakeTemplateCache(),
                      None)
    # The model namespace is "tester". No predetermined availability is found,
    # so the FakeAvailabilityFinder instance is used to find availability.
    self.assertEqual(ChannelInfo('stable', '396', 5),
                     model._GetApiAvailability())

    # These APIs have predetermined availabilities in the
    # api_availabilities.json file within CANNED_DATA.
    model._namespace.name = 'trunk_api'
    self.assertEqual(ChannelInfo('trunk', 'trunk', 'trunk'),
                     model._GetApiAvailability())

    model._namespace.name = 'dev_api'
    self.assertEqual(ChannelInfo('dev', '1500', 28),
                     model._GetApiAvailability())

    model._namespace.name = 'beta_api'
    self.assertEqual(ChannelInfo('beta', '1453', 27),
                     model._GetApiAvailability())

    model._namespace.name = 'stable_api'
    self.assertEqual(ChannelInfo('stable', '1132', 20),
                     model._GetApiAvailability())

  def testGetIntroList(self):
    model = _JSCModel(self._LoadJSON('test_file.json')[0],
                      self._CreateRefResolver('test_file_data_source.json'),
                      False,
                      FakeAvailabilityFinder(),
                      TestBranchUtility.CreateWithCannedData(),
                      self._json_cache,
                      FakeTemplateCache(),
                      None)
    expected_list = [
      { 'title': 'Description',
        'content': [
          { 'text': 'a test api' }
        ]
      },
      { 'title': 'Availability',
        'content': [
          { 'partial': 'handlebar docs/templates/private/' +
                       'intro_tables/stable_message.html',
            'version': 5
          }
        ]
      },
      { 'title': 'Permissions',
        'content': [
          { 'class': 'override',
            'text': '"tester"'
          },
          { 'text': 'is an API for testing things.' }
        ]
      },
      { 'title': 'Manifest',
        'content': [
          { 'class': 'code',
            'text': '"tester": {...}'
          }
        ]
      },
      { 'title': 'Learn More',
        'content': [
          { 'link': 'https://tester.test.com/welcome.html',
            'text': 'Welcome!'
          }
        ]
      }
    ]
    self.assertEquals(model._GetIntroTableList(), expected_list)

  def testGetAddRulesDefinitionFromEvents(self):
    events = {}
    # Missing 'types' completely.
    self.assertRaises(AssertionError, _GetAddRulesDefinitionFromEvents, events)

    events['types'] = []
    # No type 'Event' defined.
    self.assertRaises(AssertionError, _GetAddRulesDefinitionFromEvents, events)

    events['types'].append({ 'name': 'Event' })
    # 'Event' has no 'functions'.
    self.assertRaises(AssertionError, _GetAddRulesDefinitionFromEvents, events)

    events['types'][0]['functions'] = []
    # No 'functions' named 'addRules'.
    self.assertRaises(AssertionError, _GetAddRulesDefinitionFromEvents, events)

    add_rules = { "name": "addRules" }
    events['types'][0]['functions'].append(add_rules)
    self.assertEqual(add_rules, _GetAddRulesDefinitionFromEvents(events))

    events['types'][0]['functions'].append(add_rules)
    # Duplicates are an error.
    self.assertRaises(AssertionError, _GetAddRulesDefinitionFromEvents, events)

  def _FakeLoadAddRulesSchema(self):
    events = self._LoadJSON('add_rules_def_test.json')
    return _GetAddRulesDefinitionFromEvents(events)

  def testAddRules(self):
    data_source = FakeAPIAndListDataSource(
        self._LoadJSON('test_file_data_source.json'))
    dict_ = _JSCModel(self._LoadJSON('add_rules_test.json')[0],
                      self._CreateRefResolver('test_file_data_source.json'),
                      False,
                      FakeAvailabilityFinder(),
                      TestBranchUtility.CreateWithCannedData(),
                      self._json_cache,
                      FakeTemplateCache(),
                      self._FakeLoadAddRulesSchema).ToDict()
    # Check that the first event has the addRulesFunction defined.
    self.assertEquals('tester', dict_['name'])
    self.assertEquals('rules', dict_['events'][0]['name'])
    self.assertEquals('notable_name_to_check_for',
                      dict_['events'][0]['addRulesFunction'][
                          'parameters'][0]['name'])

    # Check that the second event has no addRulesFunction defined.
    self.assertEquals('noRules', dict_['events'][1]['name'])
    self.assertFalse('addRulesFunction' in dict_['events'][1])
    # But addListener should be defined.
    self.assertEquals('tester', dict_['name'])
    self.assertEquals('noRules', dict_['events'][1]['name'])
    self.assertEquals('callback',
                      dict_['events'][0]['addListenerFunction'][
                          'parameters'][0]['name'])

if __name__ == '__main__':
  unittest.main()
