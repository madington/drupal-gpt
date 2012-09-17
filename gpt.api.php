<?php

/**
 * @file
 * Hooks provided by the GPT module.
 */

/**
 * @addtogroup hooks
 * @{
 */

/**
 * Allow manipulation of page-level options for the GPT.
 *
 * @param array $options
 *   Page option associative array containing:
 *   - networkCode: The network for serving ads from, string.
 *   - targetedAdUnit: The ad unit hiearchy, string.
 *   - async: Use asynchrounous mode, boolean.
 *   - sra: Use single request architecture, boolean.
 *   - collapse: Collapse divs without creatives, boolean.
 *   - targeting: Page level targeting, multi-dimensional array, containing:
 *     - src: Contains values that should be compiled into GPT acceptable format
 *       if the "prep" key is not defined.
 *       - (dynamic): The key name for key value pairs, contains an indexed
 *         array.
 *         - (indexed)
 *           - value: String for the value.
 *           - eval: Boolean, whether to treat the value key as Javascript, if
 *             true.
 *     - prep: (optional) If prep is not defined it will be generated from src
 *       at display time.
 *       - (dynamic): The key name for the key value pair, the value is a string
 *         containing JSON compiled from the indexed "src".
 *
 */
function hook_gpt_load_page_options_alter(&$options) {
  // Fetch current page item.
  $item = menu_get_item();
  // If on a node page.
  if ($item['map'][0] == 'node' && isset($item['map'][1]->type)) {
    // Set targeting key value pair of: type to $node->type.
    $options['targeting']['src']['type'][] = array(
      'value' => $item['map'][1]->type,
      'eval' => FALSE,
    );
    // Unset prep key as src has been modified and prep should be rebuilt
    // from src's modified data.
    unset($options['targeting']['prep']);
  }
}

/**
 * Modify settings for a specific ad slot.
 *
 * @param array $ad
 *   Associate array containing:
 *   - container: The containing element ID of the ad unit, string.
 *   - size: (conditionally optional) The JSON array size value of the unit. Is
 *     optional only when "outofpage" is true.
 *   - targeting:
 *     - src: The source array for targeting, see
 *       hook_gpt_ad_page_options_alter()'s $options['targeting']['src']
 *       definition.
 *     - prep: (optional) The "compiled" src, an array of JSON strings or JSON
 *       array of strings. See hook_gpt_ad_page_options_alter()'s
 *       $options['targeting']['prep'] definition.
 *   - outofpage: Whether the ad is an interstitial or not, boolean.
 * @param array $options
 *   See hook_gpt_ad_page_options_alter()'s $options param. Note that modifying
 *   the $options param will only affect the specific ad slot being altered. Any
 *   alterations which should affect $options page wide should be performed in
 *   a hook_gpt_ad_page_options_alter() implementation.
 */
function hook_gpt_ad_solt_settings_alter(&$ad, &$options) {
  // If top ad unit on the homepage alter the ad's size.
  if (drupal_is_front_page() && $ad['container'] == 'ad-manager-ad-top-0') {
    $ad['size'] = '[728x90]';
  }
}

/**
 * @} End of "addtogroup hooks".
 */