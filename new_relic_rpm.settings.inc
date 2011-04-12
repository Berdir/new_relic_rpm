<?php

/**
 * Module settings page.
 */
function new_relic_rpm_settings(&$form_state) {
  $form = array();

  $form['ignore_cron'] = array(
    '#type' => 'select',
    '#title' => t('Cron transactions'),
    '#description' => t('How do you wish RPM to track cron tasks?'),
    '#options' => array(
      'ignore' => t('Ignore completely'),
      'bg' => t('Track as background tasks'),
      'norm' => t('Track normally'),
    ),
    '#default_value' => variable_get('new_relic_rpm_track_cron', 'norm'),
  );

  $form['module_deployment'] = array(
    '#type' => 'select',
    '#title' => t('Track module activation as deployment'),
    '#description' => t('Turning this on will create a "deployment" on the New Relic RPM dashboard each time a module is enabled or disabled. This will help you track before and after statistics.'),
    '#options' => array(
      '1' => t('Enable'),
      '0' => t('Disable'),
    ),
    '#default_value' => variable_get('new_relic_rpm_module_deployment', 0),
  );

  $form['application_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Application Name'),
    '#description' => t("If you are tracking/entering deployments via the Drupal UI, you will need to enter the *exact* RPM Application Name you have entered for the application that encompasses this Drupal installation on New Relic's website."),
    '#default_value' => variable_get('new_relic_rpm_app_name', ''),
  );

  $form['ignore_urls'] = array(
    '#type' => 'textarea',
    '#title' => t('Ignore URLs'),
    '#description' => t('Enter URLs you wish New Relic RPM to ignore. Enter one URL per line.'),
    '#default_value' => variable_get('new_relic_rpm_ignore_urls', ''),
  );

  $form['bg_urls'] = array(
    '#type' => 'textarea',
    '#title' => t('Background URLs'),
    '#description' => t('Enter URLs you wish to have tracked as background tasks. Enter one URL per line.'),
    '#default_value' => variable_get('new_relic_rpm_bg_urls', ''),
  );

  $form['exclusive_urls'] = array(
    '#type' => 'textarea',
    '#title' => t('Exclusive URLs'),
    '#description' => t('Enter URLs you wish exclusively track. This is usefull for debugging specific issues. **NOTE** Entering URLs here effectively marks all other URLs as ignored. Leave blank to disable.'),
    '#default_value' => variable_get('new_relic_rpm_exclusive_urls', ''),
  );

  $form['api_key'] = array(
    '#type' => 'textfield',
    '#title' => t('API Key'),
    '#description' => t('Enter your New Relic API key if you wish to view reports and analysis within Drupal'),
    '#default_value' => variable_get('new_relic_rpm_api_key', ''),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save Settings'),
  );

  return $form;
}

/**
 * FormAPI submit callback for module settings page.
 */
function new_relic_rpm_settings_submit($form, &$form_state) {
  $v = $form_state['values'];
  variable_set('new_relic_rpm_track_cron', $v['ignore_cron']);
  variable_set('new_relic_rpm_ignore_urls', $v['ignore_urls']);
  variable_set('new_relic_rpm_bg_urls', $v['bg_urls']);
  variable_set('new_relic_rpm_exclusive_urls', $v['exclusive_urls']);
  variable_set('new_relic_rpm_api_key', $v['api_key']);
  variable_set('new_relic_rpm_module_deployment', $v['module_deployment']);
  variable_set('new_relic_rpm_app_name', $v['application_name']);
  drupal_set_message(t('New Relic RPM settings saved.'), 'status');
}

/**
 * Manually create a deployment
 */
function new_relic_rpm_deploy(&$form_state) {
  $form = array();

  $form['deploy_user'] = array(
    '#type' => 'textfield',
    '#title' => t('Deployer/Deployment Name'),
    '#required' => TRUE,
    '#description' => t('Enter the name for this deployment of your application. This will be the name shown in your list of deployments on the New Relic RPM website.'),
  );

  $form['deploy_description'] = array(
    '#type' => 'textarea',
    '#title' => t('Deployment Description'),
    '#description' => t('Provide some notes and description regarding this deployment.'),
  );

  $form['deploy_changelog'] = array(
    '#type' => 'textarea',
    '#title' => t('Deployment Changelog'),
    '#description' => t('Provide a specific changelog for this deployment.'),
  );

  $form['deploy_revision'] = array(
    '#type' => 'textfield',
    '#title' => t('Deployment Revision'),
    '#description' => t('Add a revision number to this deployment.'),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Create Deployment'),
  );

  return $form;
}

/**
 * FormAPI sumbit callback for the manual deployment creation page.
 */
function new_relic_rpm_deploy_submit($form, &$form_state) {
  $v = $form_state['values'];
  $api_key = variable_get('new_relic_rpm_api_key', '');

  $post_vars['deployment[application_id]'] = variable_get('new_relic_rpm_app_name', '');
  $post_vars['deployment[user]'] = $v['deploy_user'];
  $post_vars['deployment[description]'] = $v['deploy_description'];
  $post_vars['deployment[changelog]'] = $v['deploy_changelog'];
  $post_vars['deployment[revision]'] = $v['deploy_revision'];
  $deployments = new_relic_rpm_curl('https://rpm.newrelic.com/deployments.xml', $api_key, $post_vars);
  if (strlen($deployments)>20) {
    drupal_set_message(t('New Relic RPM deployment created successfully'), 'status');
  }
  else {
    drupal_set_message(t('New Relic RPM deployment failed to be created. Please ensure you have your account configured on the <a href="!settings">New Relic RPM Drupal admin page</a>.', array('!settings' => url('admin/settings/new-relic-rpm'))), 'error');
  }

  watchdog('New Relic RPM', t('Module install/uninstall action logged as deployment. Log data: %details', array('%details' => $deployments)));
}
