<?php

/**
 * Helper function providing the ad provider form.
 */
function gpt_ads_form(&$form, &$form_state) {
  $settings =& $form_state['item']->settings['gpt_ads'];
  $outofpage = _gpt_ads_is_outofpage($form_state, $settings);

  // Create form here, for values use $settings[$key]
  $form['gpt_ads_size'] = array(
    '#type' => 'textfield',
    '#title' => t('Ad slot creative size'),
    '#description' => t('<p>Enter <em>width</em> x <em>height</em>, separate multiple sizes with a comma.</p><p>Example: 300x600,300x250</p>'),
    '#default_value' => !$outofpage && isset($settings['size']) ? _gpt_ads_size_to_friendly($settings['size']) : '',
    '#disabled' => $outofpage,
    '#required' => $outofpage ? FALSE : TRUE,
    '#element_validate' => array('gpt_validate_size'),
    '#prefix' => '<div id="gpt-ads-size">',
    '#suffix' => '</div>',
  );

  $form['gpt_ads_outofpage'] = array(
    '#type' => 'checkbox',
    '#title' => t('Out of page slot'),
    '#description' => t('<p>Only available to DFP Premium accounts, enable on non-premium accounts at your own risk.</p><p>If checked the creative is loaded as an interstitial.</p>'),
    '#default_value' => !isset($settings['outofpage']) ? 0 : $settings['outofpage'],
    '#ajax' => array(
      'callback' => 'gpt_ads_form_ajax_outofpage_callback',
      'wrapper' => 'gpt-ads-size',
    ),
  );

  $form['gpt_targeting_label'] = array(
    '#type' => 'item',
    '#title' => t('Slot-level customized targeting'),
    '#description' => t('Provide key-value pairs for slot-level customized targeting. Multiple values may be associated with one key.'),
    '#attributes' => array('class' => array('gpt-targeting-label')),
  );

  // Targeting fields have the same keys, we have to set #tree to access them.
  $form['#tree'] = TRUE;
  // Prepare targeting values needed.
  $targeting = isset($settings['targeting']) ? $settings['targeting'] : array();
  _gpt_targeting_prep($form_state, $targeting);
  // Add targeting form elements.
  _gpt_targeting_form_elements($form, $form_state, 'gpt_ads_form_ajax_targeting_callback');

  // Add CSS to make targeting pairs appear inline.
  _gpt_admin_attach_css();
}

/**
 * Ajax callback from changing outofpage field.
 */
function gpt_ads_form_ajax_outofpage_callback($form, $form_state) {
  return $form['gpt_ads_size'];
}

/**
 * Ajax callback from using targeting add more button.
 */
function gpt_ads_form_ajax_targeting_callback($form, $form_state) {
  return $form['gpt_targeting_wrapper'];
}

/**
 * Ad Manager ad provider form submit for AdSense Custom Search Ads.
 */
function gpt_ads_submit(&$form, &$form_state) {
  // A key of the provider name is used to prevent conflicting data if users
  // change providers for a specific ad.
  $settings =& $form_state['item']->settings['gpt_ads'];

  // Set settings for storage.
  $settings['size'] = gpt_prepare_size($form_state['values']['gpt_ads_size']);
  $settings['outofpage'] = $form_state['values']['gpt_ads_outofpage'];

  // Store targeting, iterate over elements.
  $settings['targeting']['src'] = array();
  foreach ($form_state['values']['gpt_targeting_wrapper'] as $v) {
    // Only attempt to store the key/value pair if both key and value are set.
    if (!empty($v['key']) && !empty($v['value'])) {
      $settings['targeting']['src'][$v['key']][] = array('value' => $v['value'], 'eval' => $v['eval']);
    }
  }
  $settings['targeting']['prep'] = gpt_prepare_targeting($settings['targeting']['src']);
}

/**
 * Submit handler for the add more targeting button.
 */
function gpt_form_targeting_add($form, &$form_state) {
  $form_state['gpt_targeting_num']++;
  $form_state['rebuild'] = TRUE;
}

/**
 * Set up the global config form.
 */
function gpt_settings_form(&$form, &$form_state) {
  $form['gpt'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#title' => t('Google Publisher Tags'),
    'network_code' => array(
      '#type' => 'textfield',
      '#title' => t('Network code'),
      '#description' => t('<p>The network to use for inventory, can be found in the Admin tab of DFP. Likely a 4 digit number for Premium publishers and a longer number for SB publishers.</p><p>e.g. 1234</p>'),
      '#default_value' => variable_get('gpt__network_code', ''),
      '#required' => TRUE,
    ),
    'targeted_ad_unit' => array(
      '#type' => 'textfield',
      '#title' => t('Default targeted ad unit'),
      '#description' => t('<p>The targeted ad unit and, for Premium publishers, the optional hierarchy behind it.</p><p>SB publishers can utilize one (1) level of hierarchy, while Premium publishers may utilize up to five (5) levels of hierarchy. Do not begin or end with a slash.</p><p>e.g. travel or travel/asia/food</p>'),
      '#default_value' => variable_get('gpt__targeted_ad_unit', ''),
    ),
    'async' => array(
      '#type' => 'checkbox',
      '#title' => t('Asynchronous'),
      '#description' => t('Asynchronous causes non-blocking loading of creatives, option to use Single Request Architecture, and the ability to refresh ads. Some expanding creatives may not expand, if they were not developed as Friendly iFrames.'),
      '#default_value' => variable_get('gpt__async', 1),
    ),
    'sra' => array(
      '#type' => 'checkbox',
      '#title' => t('Single Request Architecture (SRA)'),
      '#description' => t('SRA reduces ad calls to one single request, improving performance of page loads, and also allows guarantee of roadblocks.'),
      '#default_value' => variable_get('gpt__sra', 1),
    ),
    'collapse' => array(
      '#type' => 'checkbox',
      '#title' => t('Collapse empty divs'),
      '#description' => t('Collapse empty div elements when there is no creative to display.'),
      '#default_value' => variable_get('gpt__collapse', 0),
    ),
    'gpt_targeting_label' => array(
      '#type' => 'item',
      '#title' => t('Page-level customized targeting'),
      '#description' => t('Provide key-value pairs to apply to all ad slots on all pages. Multiple values may be associated with one key.'),
      '#attributes' => array('class' => array('gpt-targeting-label')),
    ),
  );

  // Prepare targeting values needed.
  $targeting = variable_get('gpt__targeting', array('src' => array()));
  _gpt_targeting_prep($form_state, $targeting);
  // Add targeting form elements.
  _gpt_targeting_form_elements($form['gpt'], $form_state, 'gpt_settings_form_ajax_targeting_callback');

  // Add CSS to make targeting pairs appear inline.
  _gpt_admin_attach_css();

  $form['#submit'][] = 'gpt_settings_submit';
}

/**
 * Ajax callback from using targeting add more button.
 */
function gpt_settings_form_ajax_targeting_callback($form, $form_state) {
  return $form['gpt']['gpt_targeting_wrapper'];
}

/**
 * Global config form submission handler.
 */
function gpt_settings_submit(&$form, &$form_state) {
  $keys = array('network_code', 'targeted_ad_unit', 'async', 'sra', 'collapse');
  foreach ($keys as $key) {
    variable_set('gpt__' . $key, $form_state['values']['gpt'][$key]);
  }

  // Store targeting, iterate over elements.
  $targeting = array('src' => array());
  foreach ($form_state['values']['gpt']['gpt_targeting_wrapper'] as $v) {
    // Only attempt to store the key/value pair if both key and value are set.
    if (!empty($v['key']) && !empty($v['value'])) {
      $targeting['src'][$v['key']][] = array('value' => $v['value'], 'eval' => $v['eval']);
    }
  }
  $targeting['prep'] = gpt_prepare_targeting($targeting['src']);
  variable_set('gpt__targeting', $targeting);
}

/**
 * Helper to attach admin CSS once per page load.
 */
function _gpt_admin_attach_css() {
  static $loaded = FALSE;
  if (!$loaded) {
    drupal_add_css(drupal_get_path('module', 'gpt') . '/css/admin.css', array('preprocess' => FALSE));
    $loaded = TRUE;
  }
}

/**
 * Determine if outofpage is set.
 *
 * @param array $form_state
 * @param array $settings
 *
 * @return boolean
 */
function _gpt_ads_is_outofpage($form_state, $settings = array()) {
  if (isset($form_state['values'])) {
    return $form_state['values']['gpt_ads_outofpage'];
  }
  elseif (isset($settings['outofpage'])) {
    return $settings['outofpage'];
  }

  return FALSE;
}

/**
 * Convert JSON size array to more human friendly width by height format.
 *
 * @param string|NULL $json
 *   JSON array in the format of [width,height] or
 *   [[width,height],[width,height]...]
 *
 * @return string|void
 */
function _gpt_ads_size_to_friendly($json = NULL) {
  if (!$json) {
    return;
  }

  // Match the values from the JSON.
  if (preg_match_all('#[1-9][0-9]*,[1-9][0-9]*#', $json, $matches)) {
    // Substitute comma in between width and height for x.
    foreach ($matches[0] as &$v) {
      $v = str_replace(',', 'x', $v);
    }
    $matches = array_shift($matches);
    // Implode multiple matches with comma separation.
    return implode(',', $matches);
  }
  else {
    return;
  }
}

/**
 * Element validate callback for GPT provider size field.
 *
 * @param array $element
 * @param array $form_state
 * @param array $form
 *
 * @return void
 */
function _gpt_ads_validate_size($element, &$form_state, $form) {
  // If out of page is set, clear size and don't validate.
  if (_gpt_ads_is_outofpage($form_state)) {
    $form_state['values']['gpt_ads_size'] = '';
    return;
  }
  
  // Validate string.
  $element['#value'] = str_replace(' ', '', $element['#value']);
  if (!preg_match('#^[1-9][0-9]*x[1-9][0-9]*(?:,[1-9][0-9]*x[1-9][0-9]*)*$#', $element['#value'])) {
    _gpt_admin_attach_css();
    form_error($element, t('Ad slot creative size does not match the expected format.'));
  }
}

/**
 * Elements
 *
 * @param array $form
 * @param array $form_state
 * @param string $ajax_callback
 *   The function to callback upon AJAX add more button.
 *
 * @return void
 */
function _gpt_targeting_form_elements(&$form, &$form_state, $ajax_callback) {
  $form['gpt_targeting_wrapper'] = array(
    '#prefix' => '<div id="gpt-targeting">',
    '#suffix' => '</div>',
    '#tree' => TRUE,
  );

  // Determine if the user is allowed to alter/create Javascript targeting.
  $js_perm = user_access('gpt set javascript');
  // Build the fields with the proper number of targeting fields.
  for ($i = 0; $i < $form_state['gpt_targeting_num']; $i++) {
    $default = isset($form_state['gpt_targeting_defaults'][$i]) ? $form_state['gpt_targeting_defaults'][$i] : FALSE;
    $form['gpt_targeting_wrapper'][$i] = array(
      '#prefix' => '<div class="inline clearfix">',
      '#suffix' => '</div>',
      '#element_validate' => array('gpt_validate_targeting'),
    );
    $form['gpt_targeting_wrapper'][$i]['key'] = array(
      '#type' => 'textfield',
      '#title' => t('Key'),
      '#size' => 15,
      '#default_value' => $default ? $default['key'] : '',
    );
    $form['gpt_targeting_wrapper'][$i]['value'] = array(
      '#type' => 'textfield',
      '#title' => t('Value'),
      '#size' => 25,
      '#default_value' => $default ? $default['value'] : '',
    );
    // Prevent users without the appropriate permission from setting Javascript.
    if ($default['eval'] && !$js_perm) {
      $form['gpt_targeting_wrapper'][$i]['key']['#disabled'] = TRUE;
      $form['gpt_targeting_wrapper'][$i]['value']['#disabled'] = TRUE;
    }
    $form['gpt_targeting_wrapper'][$i]['eval'] = array(
      '#type' => 'checkbox',
      '#title' => t('Value is Javascript'),
      '#default_value' => $default ? $default['eval'] : '',
      '#disabled' => !$js_perm,
    );
  }

  $form['gpt_targeting_eval_desc'] = array(
    '#type' => 'item',
    '#description' => t('If the <em>value is Javascript</em> setting is used for a value, and the value is not valid Javascript, errors will be caused on the site which will upredictably break site functionality.'),
    // #type item doesn't support attributes so we have to use prefix/suffix.
    '#prefix' => '<div class="gpt-targeting-eval-desc">',
    '#suffix' => '</div>',
  );

  // Build add button.
  $form['gpt_targeting_add'] = array(
    '#type' => 'submit',
    '#value' => t('Add more'),
    '#submit' => array('gpt_form_targeting_add'),
    '#attributes' => array('class' => array('gpt-targeting-add')),
    '#ajax' => array(
      'callback' => $ajax_callback,
      'wrapper' => 'gpt-targeting',
    ),
  );
}

/**
 * Prepare targeting values for use in form builder.
 *
 * @param array $form_state
 * @param array $targeting
 *   Pre-stored targeting settings.
 *
 * @return void
 */
function _gpt_targeting_prep(&$form_state, $targeting = array()) {
  // If targeting num is already set prep has already been completed.
  if (!empty($form_state['gpt_targeting_num'])) {
    return;
  }

  // Attempt to load stored settings.
  if (!empty($targeting)) {
    // If loading stored settings reset targeting num and defaults.
   $form_state['gpt_targeting_num'] = $i = 0;

    // Iterate over stored values.
    foreach ($targeting['src'] as $key => $j) {
      foreach ($j as $v) {
        $form_state['gpt_targeting_defaults'][$i] = array(
          'key' => $key,
          'value' => $v['value'],
          'eval' => $v['eval'],
        );
        $i++;
      }
    }

    // Set targeting num so that form builder will show correct fields.
    $form_state['gpt_targeting_num'] = $i;
  }

  // Initialize form state's gpt_targeting_num if not set.
  if (empty($form_state['gpt_targeting_num'])) {
    $form_state['gpt_targeting_num'] = 1;
  }
}

/**
 * Element validate callback for GPT targeting fields.
 *
 * Converts targeting fields to the expected JSON GPT format.
 *
 * @param array $element
 * @param array $form_state
 * @param array $form
 *
 * @return void
 */
function _gpt_validate_targeting($element, &$form_state, $form) {
  if (!isset($form_state['targeting'])) {
    $form_state['targeting'] = array();
  }

  $key = $element['key']['#value'];
  $value = $element['value']['#value'];
  $eval = $element['eval']['#value'];

  // If key or value has been set, but not both, then set an error on the empty one.
  if (!(!empty($key) && !empty($value)) && !(empty($key) && empty($value))) {
    // Attach admin CSS if there is an error.
    _gpt_admin_attach_css();

    // Prepare form_set_error name for use in errors below.
    $name = implode('][', $element['#parents']) . '][';
    if (empty($key)) {
      form_set_error($name . 'key', t('Targeting key for value %value has been left empty.', array('%value' => $value)));
    }
    else {
      form_set_error($name . 'value', t('Targeting value for key %key has been left empty.', array('%key' => $key)));
    }
  }
}
