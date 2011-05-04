<?php
/**
 * @file
 * Reporting page callbacks.
 */

/**
 * Main page for RPM reporting. List all the applications on this API Key.
 */
function new_relic_rpm_reporting() {

  // If no API key is set, break here and error out.
  $api_key = variable_get('new_relic_rpm_api_key', '');
  if (empty($api_key)) {
    drupal_set_message(t('You need to enter your New Relic API key from your New Relic account settings page before you are able to view reports within Drupal. Visit the <a href="!settings">New Relic RPM Drupal admin page</a> to enter your API key.', array('!settings' => url('admin/settings/new-relic-rpm'))), 'error');
    return '<h2>' . t('No API key found.') . '</h2>';
  }

  // Get basic app health. This is also our first check for a bad key/access denied.
  // Only hit the REST API every 60 seconds.
  if ($_SESSION['new_relic_rpm_health_time'] < $_SERVER['REQUEST_TIME'] - 60 || !$_SESSION['new_relic_rpm_health_xml']) {
    $app_health = new_relic_rpm_curl('https://rpm.newrelic.com/accounts.xml?include=application_health');
  }
  else {
    $app_health = $_SESSION['new_relic_rpm_health_xml'];
  }
  // Error out of the return is False, store data if it is good.
  if (!$app_health) {
    drupal_set_message(t('The New Relic REST API has denied your key. Either the key you entered on the <a href="!settings">New Relic RPM Drupal admin page</a> is incorrect, or you have not enabled API access for this application within the New Relic RPM webiste.', array('!settings' => url('admin/settings/new-relic-rpm'))), 'error');
    return '<h2>' . t('API access denied.') . '</h2>';
  }
  else {
    $_SESSION['new_relic_rpm_health_time'] = $_SERVER['REQUEST_TIME'];
    $_SESSION['new_relic_rpm_health_xml'] = $app_health;
  }

  return new_relic_rpm_render_health($app_health);
}

/**
 * Load the details for a specfic application.
 */
function new_relic_rpm_reporting_details($cust_id, $app_id) {

  $output = '';

  // If no API key is set, break here and error out.
  $api_key = variable_get('new_relic_rpm_api_key', '');
  if (empty($api_key)) {
    drupal_set_message(t('You need to enter your New Relic API key from your New Relic account settings page before you are able to view reports within Drupal. Visit the <a href="!settings">New Relic RPM Drupal admin page<a/> to enter your API key.', array('!settings' => url('admin/settings/new-relic-rpm'))), 'error');
    return '<h2>' . t('No API key found.') . '</h2>';
  }

  // Only hit the REST API every 60 seconds.
  if ($_SESSION['new_relic_rpm_dash_time'] < $_SERVER['REQUEST_TIME'] - 60 || !$_SESSION['new_relic_rpm_dash_xml']) {
    $app_dashboard = new_relic_rpm_curl('https://rpm.newrelic.com/application_dashboard?application_id=' . $app_id);
  }
  else {
    $app_dashboard = $_SESSION['new_relic_rpm_dash_xml'];
  }

  // Error out if value is false, save cached copy of XML if it is good.
  if (!$app_dashboard) {
    drupal_set_message(t('The New Relic REST API has denied your key. Either the key you entered on the <a href="!settings">New Relic RPM Drupal admin page</a> is incorrect, or you have not enabled API access for this application within the New Relic RPM webiste.', array('!settings' => url('admin/settings/new-relic-rpm'))), 'error');
    return '<h2>' . t('API access denied.') . '</h2>';
  }
  else {
    $_SESSION['new_relic_rpm_dash_time'] = $_SERVER['REQUEST_TIME'];
    $_SESSION['new_relic_rpm_dash_xml'] = $app_dashboard;
  }

  $output .= $app_dashboard;

  //$post_vars['deployment[application_id]'] = check_plain($_GET['app_name']);
  //$deployments = new_relic_rpm_curl('https://rpm.newrelic.com/deployments.xml', $post_vars);
  //$output .= '<pre>'.htmlentities(print_r($deployments, TRUE)).'</pre>';
  $output .= new_relic_rpm_render_actions($cust_id, $app_id);

  return $output;
}

/**
 * Render the application health display.
 * TODO -- Figure out the threshold information
 */
function new_relic_rpm_render_health($app_health) {

  $app_obj = simplexml_load_string($app_health);
  $cust_id = (string)$app_obj->account->id;

  $apps = $app_obj->account->applications;
  foreach ($apps as $app){
    foreach ($app as $data) {

      foreach ($data->{"threshold-values"} as $val) {
          foreach ($val->threshold_value as $v) {

            $app_name = (string)$data->name;
            $attr_name = (string)$v->attributes()->name;
            $app_data[$app_name][$attr_name]['val'] = (string)$v->attributes()->formatted_metric_value;
            $app_data[$app_name][$attr_name]['mval'] = (string)$v->attributes()->metric_value;
            $app_data[$app_name][$attr_name]['thresh'] = (string)$v->attributes()->threshold_value;
            $app_data[$app_name]['AppID'] = (string)$data->id;

          }

      }
    }
  }

  $table['header'] = array(
    'App Name',
    'App ID',
    'ApDex',
    'Resp. Time',
    'Errors %',
    'Error Rate (EPM)',
    'Throughput (rpm)',
    'App Busy %',
    'DB %',
    'CPU',
    'Memory',
    'Details',
  );

  foreach ($app_data as $k => $v) {
    $table['rows'][] = array(
      $k,
      $v['AppID'],
      $v['Apdex']['mval'] > $v['Apdex']['thresh'] ? '<span class="error">'.$v['Apdex']['val'].'</span>' : $v['Apdex']['val'],
      $v['Response Time']['mval'] > $v['Response Time']['thresh'] ? '<span class="error">'.$v['Response Time']['val'].'</span>' : $v['Response Time']['val'],
      $v['Errors']['mval'] > $v['Errors']['thresh'] ? '<span class="error">'.$v['Errors']['val'].'</span>' : $v['Errors']['val'],
      $v['Error Rate']['mval'] > $v['Error Rate']['thresh'] ? '<span class="error">'.$v['Error Rate']['val'].'</span>' : $v['Error Rate']['val'],
      $v['Throughput']['mval'] > $v['Throughput']['thresh'] ? '<span class="error">'.$v['Throughput']['val'].'</span>' : $v['Throughput']['val'],
      $v['Application Busy']['mval'] > $v['Application Busy']['thresh'] ? '<span class="error">'.$v['Application Busy']['val'].'</span>' : $v['Application Busy']['val'],
      $v['DB']['mval'] > $v['DB']['thresh'] ? '<span class="error">'.$v['DB']['val'].'</span>' : $v['DB']['val'],
      $v['CPU']['mval'] > $v['CPU']['thresh'] ? '<span class="error">'.$v['CPU']['val'].'</span>' : $v['CPU']['val'],
      $v['Memory']['mval'] > $v['Memory']['thresh'] ? '<span class="error">'.$v['Memory']['val'].'</span>' : $v['Memory']['val'],
      l('Details', 'admin/reports/new-relic-rpm/details/'.$cust_id.'/'.$v['AppID'], array('query'=>array('app_name'=>$k))),
    );
  }

  return theme_table($table['header'], $table['rows']);
}

/**
 * Render the quick actions menu.
 */
function new_relic_rpm_render_actions($cust_id, $app_id) {

  $urls['overview'] = "https://rpm.newrelic.com/accounts/{$cust_id}/applications/{$app_id}";
  $urls['web_transactions'] = "https://rpm.newrelic.com/accounts/{$cust_id}/applications/{$app_id}/transactions";
  $urls['databases'] = "https://rpm.newrelic.com/accounts/{$cust_id}/applications/{$app_id}/databases";
  $urls['bg_tasks'] = "https://rpm.newrelic.com/accounts/{$cust_id}/applications/{$app_id}/background_tasks?task=all";
  $urls['traced_errors'] = "https://rpm.newrelic.com/accounts/{$cust_id}/applications/{$app_id}/traced_errors";
  $urls['app_settings'] = "https://rpm.newrelic.com/accounts/{$cust_id}/applications/{$app_id}/edit";
  $urls['alerts'] = "https://rpm.newrelic.com/accounts/{$cust_id}/applications/{$app_id}/edit_alerts";
  $urls['deployments'] = "https://rpm.newrelic.com/accounts/{$cust_id}/applications/{$app_id}/deployments";

  $output = '<h2>Quick Links for this Application</h2>';
  $output .= '<p>Links open New Relic\'s website in a new window.</p>';
  $output .= '<ul>';
  $output .= '<li>'.l('Overview', $urls['overview'], array('attributes'=>array('target'=>'_blank'))).'</li>';
  $output .= '<li>'.l('Web Transactions', $urls['web_transactions'], array('attributes'=>array('target'=>'_blank'))).'</li>';
  $output .= '<li>'.l('Database Activity', $urls['databases'], array('attributes'=>array('target'=>'_blank'))).'</li>';
  $output .= '<li>'.l('Background Tasks', $urls['bg_tasks'], array('attributes'=>array('target'=>'_blank'))).'</li>';
  $output .= '<li>'.l('Traced Errors', $urls['traced_errors'], array('attributes'=>array('target'=>'_blank'))).'</li>';
  $output .= '<li>'.l('App Settings', $urls['app_settings'], array('attributes'=>array('target'=>'_blank'))).'</li>';
  $output .= '<li>'.l('App Alerts & Thresholds', $urls['alerts'], array('attributes'=>array('target'=>'_blank'))).'</li>';
  $output .= '<li>'.l('Deployments', $urls['deployments'], array('attributes'=>array('target'=>'_blank'))).'</li>';
  $output .= '</ul>';

  return $output;
}
