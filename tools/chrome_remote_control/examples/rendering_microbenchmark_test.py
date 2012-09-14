#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import os
import re
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), ".."))

import chrome_remote_control

def Main(args):
  options = chrome_remote_control.BrowserOptions()
  parser = options.CreateParser(
      "rendering_microbenchmark_test.py <sitelist>")
  # TODO(nduca): Add test specific options here, if any.
  options, args = parser.parse_args(args)
  if len(args) != 1:
    parser.print_usage()
    return 255

  urls = []
  with open(args[0], "r") as f:
    for url in f.readlines():
      url = url.strip()
      if not re.match("(.+)://", url):
        url = "http://%s" % url
      urls.append(url)

  options.extra_browser_args.append("--enable-gpu-benchmarking")
  browser_to_create = chrome_remote_control.FindBrowser(options)
  if not browser_to_create:
    sys.stderr.write("No browser found! Supported types: %s" %
        chrome_remote_control.GetAllAvailableBrowserTypes())
    return 255
  with browser_to_create.Create() as b:
    with b.ConnectToNthTab(0) as tab:
      # Check browser for benchmark API. Can only be done on non-chrome URLs.
      tab.page.Navigate("http://www.google.com")
      import time
      time.sleep(2)
      tab.WaitForDocumentReadyStateToBeComplete()
      if tab.runtime.Evaluate("window.chrome.gpuBenchmarking === undefined"):
        print "Browser does not support gpu benchmarks API."
        return 255

      if tab.runtime.Evaluate(
          "window.chrome.gpuBenchmarking.runRenderingBenchmarks === undefined"):
        print "Browser does not support rendering benchmarks API."
        return 255

      # Run the test. :)
      first_line = []
      def DumpResults(url, results):
        if len(first_line) == 0:
          cols = ["url"]
          for r in results:
            cols.append(r["benchmark"])
          print ",".join(cols)
          first_line.append(0)
        cols = [url]
        for r in results:
          cols.append(str(r["result"]))
        print ",".join(cols)

      for u in urls:
        tab.page.Navigate(u)
        tab.WaitForDocumentReadyStateToBeInteractiveOrBetter()
        results = tab.runtime.Evaluate(
            "window.chrome.gpuBenchmarking.runRenderingBenchmarks();")
        DumpResults(url, results)

  return 0

if __name__ == "__main__":
  sys.exit(Main(sys.argv[1:]))
