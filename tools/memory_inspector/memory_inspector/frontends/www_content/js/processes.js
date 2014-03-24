// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

processes = new (function() {

this.PS_INTERVAL_SEC_ = 2;
this.DEV_STATS_INTERVAL_SEC_ = 2;
this.PROC_STATS_INTERVAL_SEC_ = 1;

this.selProcUri_ = null;
this.selProcName_ = null;
this.psTable_ = null;
this.psTableData_ = null;
this.memChart_ = null;
this.memChartData_ = null;
this.cpuChart_ = null;
this.cpuChartData_ = null;
this.procCpuChart_ = null;
this.procCpuChartData_ = null;
this.procMemChart_ = null;
this.procMemChartData_ = null;

this.onDomReady_ = function() {
  $('#device_tabs').tabs();
  $('#device_tabs').on('tabsactivate', this.redrawPsStats_.bind(this));
  $('#device_tabs').on('tabsactivate', this.redrawDevStats_.bind(this));

  // Initialize the toolbar.
  $('#ps-quick_snapshot').button({icons:{primary: 'ui-icon-image'}})
      .click(this.snapshotSelectedProcess_.bind(this));
  $('#ps-dump_mmaps').button({icons:{primary: 'ui-icon-calculator'}})
      .click(this.dumpSelectedProcessMmaps_.bind(this));

  // Create the process table.
  this.psTable_ = new google.visualization.Table($('#ps-table')[0]);
  google.visualization.events.addListener(
      this.psTable_, 'select', this.onPsTableRowSelect_.bind(this));
  $('#ps-table').on('dblclick', this.snapshotSelectedProcess_.bind(this));

  // Create the device stats charts.
  this.memChart_ = new google.visualization.PieChart($('#os-mem_chart')[0]);
  this.cpuChart_ = new google.visualization.BarChart($('#os-cpu_chart')[0]);

  // Create the selected process stats charts.
  this.procCpuChart_ =
      new google.visualization.ComboChart($('#proc-cpu_chart')[0]);
  this.procMemChart_ =
      new google.visualization.ComboChart($('#proc-mem_chart')[0]);
};

this.getSelectedProcessURI = function() {
  return this.selProcUri_;
};

this.snapshotSelectedProcess_ = function() {
  if (!this.selProcUri_)
    return alert('Must select a process!');
  mmap.dumpMmaps(this.selProcUri_, true);
  rootUi.showTab('prof');
};

this.dumpSelectedProcessMmaps_ = function() {
  if (!this.selProcUri_)
    return alert('Must select a process!');
  mmap.dumpMmaps(this.selProcUri_, false);
  rootUi.showTab('mm');
};

this.refreshPsTable = function() {
  var targetDevUri = devices.getSelectedURI();
  if (!targetDevUri)
    return this.stopPsTable();

  var showAllParam = $('#ps-show_all').prop('checked') ? '?all=1' : '';
  webservice.ajaxRequest('/ps/' + targetDevUri + showAllParam,
                         this.onPsAjaxResponse_.bind(this),
                         this.stopPsTable.bind(this));
};

this.startPsTable = function() {
  timers.start('ps_table',
               this.refreshPsTable.bind(this),
               this.PS_INTERVAL_SEC_);
};

this.stopPsTable = function() {
  this.selProcUri_ = null;
  this.selProcName_ = null;
  timers.stop('ps_table');
};

this.onPsTableRowSelect_ = function() {
  var targetDevUri = devices.getSelectedURI();
  if (!targetDevUri)
    return;

  var sel = this.psTable_.getSelection();
  if (!sel.length || !this.psTableData_)
    return;
  var pid = this.psTableData_.getValue(sel[0].row, 0);
  this.selProcUri_ = targetDevUri + '/' + pid;
  this.selProcName_ = this.psTableData_.getValue(sel[0].row, 1);
  this.startSelectedProcessStats();
};

this.onPsAjaxResponse_ = function(data) {
  this.psTableData_ = new google.visualization.DataTable(data);
  this.redrawPsTable_();
};

this.redrawPsTable_ = function(data) {
  if (!this.psTableData_)
    return;

  // Redraw table preserving sorting info.
  var sort = this.psTable_.getSortInfo() || {column: -1, ascending: false};
  this.psTable_.draw(this.psTableData_, {sortColumn: sort.column,
                                         sortAscending: sort.ascending});
};

this.refreshDeviceStats = function() {
  var targetDevUri = devices.getSelectedURI();
  if (!targetDevUri)
    return this.stopDeviceStats();

  webservice.ajaxRequest('/stats/' + targetDevUri,
                         this.onDevStatsAjaxResponse_.bind(this),
                         this.stopDeviceStats.bind(this));
};

this.startDeviceStats = function() {
  timers.start('device_stats',
               this.refreshDeviceStats.bind(this),
               this.DEV_STATS_INTERVAL_SEC_);
};

this.stopDeviceStats = function() {
  timers.stop('device_stats');
};

this.onDevStatsAjaxResponse_ = function(data) {
  this.memChartData_ = new google.visualization.DataTable(data.mem);
  this.cpuChartData_ = new google.visualization.DataTable(data.cpu);
  this.redrawDevStats_();
};

this.redrawDevStats_ = function(data) {
  if (!this.memChartData_ || !this.cpuChartData_)
    return;

  this.memChart_.draw(this.memChartData_,
                       {title: 'System Memory Usage (MB)', is3D: true});
  this.cpuChart_.draw(this.cpuChartData_,
                       {title: 'CPU Usage',
                        isStacked: true,
                        hAxis: {maxValue: 100, viewWindow: {max: 100}}});
};

this.refreshSelectedProcessStats = function() {
  if (!this.selProcUri_)
    return this.stopSelectedProcessStats();

  webservice.ajaxRequest('/stats/' + this.selProcUri_,
                         this.onPsStatsAjaxResponse_.bind(this),
                         this.stopSelectedProcessStats.bind(this));
};

this.startSelectedProcessStats = function() {
  timers.start('proc_stats',
               this.refreshSelectedProcessStats.bind(this),
               this.PROC_STATS_INTERVAL_SEC_);
  $('#device_tabs').tabs('option', 'active', 1);
};

this.stopSelectedProcessStats = function() {
  timers.stop('proc_stats');
};

this.onPsStatsAjaxResponse_ = function(data) {
  this.procCpuChartData_ = new google.visualization.DataTable(data.cpu);
  this.procMemChartData_ = new google.visualization.DataTable(data.mem);
  this.redrawPsStats_();
};

this.redrawPsStats_ = function() {
  if (!this.procCpuChartData_ || !this.procMemChartData_)
    return;

  this.procCpuChart_.draw(this.procCpuChartData_, {
      title: 'CPU stats for ' + this.selProcUri_,
      seriesType: 'line',
      vAxes: {0: {title: 'CPU %', maxValue: 100}, 1: {title: '# Threads'}},
      series: {1: {type: 'bars', targetAxisIndex: 1}},
      hAxis: {title: 'Run Time'},
      legend: {alignment: 'end'},
  });
  this.procMemChart_.draw(this.procMemChartData_, {
      title: 'Memory stats for ' + this.selProcUri_,
      seriesType: 'line',
      vAxes: {0: {title: 'VM Rss KB'}, 1: {title: '# Page Faults'}},
      series: {1: {type: 'bars', targetAxisIndex: 1}},
      hAxis: {title: 'Run Time'},
      legend: {alignment: 'end'},
  });
};

this.redraw = function() {
  this.redrawPsTable_();
  if ($('#device_tabs').tabs('option', 'active') == 0)
    this.redrawDevStats_();
  else
    this.redrawPsStats_();
};

$(document).ready(this.onDomReady_.bind(this));

})();