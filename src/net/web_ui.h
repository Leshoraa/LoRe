/**
 * @file web_ui.h
 * @brief Production Web UI dashboard and telemetry control interface for LoRe.
 * @details Balanced Modern Bento Grid Design System:
 *          - Integrated Top Segmented Tab Control aligned with dashboard grid.
 *          - Interactive 2D Gaze Steering Pad with real-time stimulus tracking.
 *          - Pure geometric typography with rounded numerals (robust offline system fallback).
 *          - Proportional multi-column metric tiles on both mobile and desktop screens.
 *          - Cyber radar HUD canvas with real-time target trajectory and coordinate feedback.
 */

#ifndef WEB_UI_H
#define WEB_UI_H

#include <pgmspace.h>

static const char HTML_PAGE[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>LoRe Ocular & Emotion Control</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Plus+Jakarta+Sans:wght@500;600;700;800&display=swap" rel="stylesheet">
  <style>
    :root {
      /* Material 3 Expressive Color System (Light Baseline & Surfaces) */
      --md-sys-color-surface: #f8f9fd;
      --md-sys-color-surface-dim: #d9dbdf;
      --md-sys-color-surface-bright: #ffffff;
      --md-sys-color-surface-container-lowest: #ffffff;
      --md-sys-color-surface-container-low: #f3f4f8;
      --md-sys-color-surface-container: #eeeff3;
      --md-sys-color-surface-container-high: #e8e9ee;
      --md-sys-color-surface-container-highest: #e2e3e8;

      /* Material 3 Expressive On-Surface & Typography Roles */
      --md-sys-color-on-surface: #191c20;
      --md-sys-color-on-surface-variant: #43474e;
      --md-sys-color-outline: #73777f;
      --md-sys-color-outline-variant: #c3c7cf;

      /* Material 3 Expressive Primary & Accent Roles */
      --md-sys-color-primary: #191c20;
      --md-sys-color-on-primary: #ffffff;
      --md-sys-color-primary-container: #dce2ed;
      --md-sys-color-on-primary-container: #111827;

      /* Material 3 Expressive Secondary & Tertiary Roles */
      --md-sys-color-secondary: #565f6c;
      --md-sys-color-on-secondary: #ffffff;
      --md-sys-color-secondary-container: #dbe3ee;
      --md-sys-color-on-secondary-container: #131d27;
      --md-sys-color-tertiary: #386663;
      --md-sys-color-tertiary-container: #bcebe5;
      --md-sys-color-on-tertiary-container: #002023;

      /* Semantic System Mapping */
      --bg-page: var(--md-sys-color-surface);
      --bg-card: var(--md-sys-color-surface-bright);
      --bg-surface: var(--md-sys-color-surface-container);
      --bg-surface-hover: var(--md-sys-color-surface-container-high);
      --bg-subtle: var(--md-sys-color-surface-container-low);
      --text-main: var(--md-sys-color-on-surface);
      --text-muted: var(--md-sys-color-on-surface-variant);
      --text-subtle: var(--md-sys-color-outline);
      --border-card: var(--md-sys-color-outline-variant);
      --border-subtle: var(--md-sys-color-surface-container-highest);
      --accent-dark: var(--md-sys-color-primary);
      --accent-dark-hover: #2d3137;
      --shadow-card: 0 1px 3px rgba(0, 0, 0, 0.04), 0 1px 2px rgba(0, 0, 0, 0.02);
      
      /* Material 3 Expressive Corner Radius Scale */
      --radius-card: 20px;
      --radius-sub: 14px;
      --radius-control: 10px;
    }

    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
      -webkit-tap-highlight-color: transparent;
      outline: none;
    }

    button {
      outline: none;
      border: none;
      background: none;
      -webkit-appearance: none;
      -moz-appearance: none;
      appearance: none;
    }

    button:focus, button:focus-visible, button:active {
      outline: none;
    }

    body {
      background-color: var(--bg-page);
      color: var(--text-main);
      font-family: "Plus Jakarta Sans", -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      min-height: 100vh;
      padding: 20px 16px;
      -webkit-font-smoothing: antialiased;
      -moz-osx-font-smoothing: grayscale;
      display: flex;
      flex-direction: column;
      align-items: center;
    }

    .app-wrapper {
      width: 100%;
      max-width: 920px;
      display: flex;
      flex-direction: column;
      gap: 14px;
    }

    /* Top Navigation Bar */
    .top-controls {
      display: flex;
      align-items: center;
      justify-content: space-between;
      width: 100%;
      gap: 12px;
    }

    /* Material 3 Expressive Tab Navigation */
    .tab-segmented-control {
      display: inline-flex;
      align-items: center;
      background-color: var(--bg-card);
      border: 1px solid var(--border-card);
      border-radius: var(--radius-card);
      padding: 4px;
      gap: 4px;
      box-shadow: var(--shadow-card);
      position: relative;
    }

    .tab-btn {
      position: relative;
      border: none;
      outline: none;
      background: transparent;
      padding: 8px 18px 10px;
      border-radius: 20px; /* Full Rounded Pill (proportional to height) */
      font-family: inherit;
      font-size: 12.5px;
      font-weight: 600;
      color: var(--text-muted);
      cursor: pointer;
      transition: color 0.2s ease,
                  background-color 0.2s ease,
                  border-radius 0.25s cubic-bezier(0.2, 0, 0, 1),
                  transform 0.15s ease;
      white-space: nowrap;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      gap: 7px;
      user-select: none;
    }

    .tab-btn .tab-icon {
      width: 16px;
      height: 16px;
      stroke: currentColor;
      transition: transform 0.2s ease;
      overflow: visible;
      flex-shrink: 0;
      display: inline-block;
    }

    .tab-btn:hover {
      background-color: var(--bg-surface);
      color: var(--text-main);
      border-radius: 12px; /* M3 Medium on Hover */
    }

    .tab-btn:active {
      transform: scale(0.97);
      border-radius: 9px; /* Small on Click */
    }

    .tab-btn.active {
      color: var(--text-main);
      font-weight: 700;
      background-color: var(--bg-surface);
      border-radius: 12px; /* M3 Medium when Selected */
    }

    /* M3 Expressive Inset Active Indicator Line */
    .tab-btn::after {
      content: '';
      position: absolute;
      bottom: 2px;
      left: 18%;
      right: 18%;
      height: 3px;
      background-color: var(--accent-dark);
      border-radius: 3px 3px 0 0;
      transform: scaleX(0);
      opacity: 0;
      transition: transform 0.28s cubic-bezier(0.2, 0, 0, 1), opacity 0.2s ease;
    }

    .tab-btn.active::after {
      transform: scaleX(1);
      opacity: 1;
    }

    .mode-pill {
      background-color: var(--bg-card);
      border: 1px solid var(--border-card);
      border-radius: var(--radius-control);
      padding: 6px 14px;
      font-size: 12px;
      font-weight: 600;
      color: var(--text-muted);
      white-space: nowrap;
      box-shadow: var(--shadow-card);
    }

    /* Bento Grid Structure */
    .bento-grid {
      display: grid;
      grid-template-columns: 1.28fr 1fr;
      gap: 14px;
      width: 100%;
      align-items: start;
    }

    .bento-col {
      display: flex;
      flex-direction: column;
      gap: 12px;
    }

    .bento-card {
      background-color: var(--bg-card);
      border: 1px solid var(--border-card);
      border-radius: var(--radius-card);
      padding: 16px;
      box-shadow: var(--shadow-card);
      display: flex;
      flex-direction: column;
      height: auto;
    }

    .bento-card-header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      margin-bottom: 12px;
    }

    .card-title {
      font-size: 13.5px;
      font-weight: 700;
      color: var(--text-main);
      letter-spacing: -0.01em;
    }

    .card-badge {
      font-size: 11px;
      font-weight: 600;
      color: var(--text-muted);
      background-color: var(--bg-surface);
      border: none;
      padding: 3px 8px;
      border-radius: 5px;
    }

    /* Gaze Steering Pad & Radar Viewport - Monochrome B&W System */
    .gaze-viewport {
      position: relative;
      width: 100%;
      aspect-ratio: 4 / 3;
      background-color: var(--bg-surface);
      border: 1px solid var(--border-card);
      border-radius: var(--radius-sub);
      box-shadow: inset 0 2px 5px rgba(0, 0, 0, 0.03);
      overflow: hidden;
      display: flex;
      justify-content: center;
      align-items: center;
      cursor: crosshair;
      user-select: none;
      touch-action: none;
      transition: border-color 0.2s ease, box-shadow 0.2s ease;
    }

    .gaze-viewport:hover {
      border-color: var(--md-sys-color-outline);
    }

    .gaze-viewport:active {
      box-shadow: inset 0 2px 7px rgba(0, 0, 0, 0.07);
    }

    #gaze-canvas {
      position: absolute;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      display: block;
      z-index: 2;
    }

    .gaze-hint {
      margin-top: 8px;
      font-size: 11px;
      color: var(--text-muted);
      text-align: center;
      font-weight: 600;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 6px;
      letter-spacing: -0.01em;
    }

    .btn-reset-gaze {
      font-family: inherit;
      font-size: 11px;
      font-weight: 600;
      color: var(--text-main);
      background-color: var(--bg-surface);
      border: 1px solid var(--border-card);
      padding: 3px 9px;
      border-radius: 6px;
      cursor: pointer;
      display: inline-flex;
      align-items: center;
      gap: 4px;
      transition: background-color 0.2s ease, color 0.2s ease, border-color 0.2s ease, transform 0.15s ease;
    }

    .btn-reset-gaze:hover {
      background-color: var(--accent-dark);
      color: #ffffff;
      border-color: var(--accent-dark);
    }

    .btn-reset-gaze:active {
      transform: scale(0.96);
    }

    /* Telemetry Metrics Grid - Strokeless Inner Cards */
    .metrics-grid {
      display: grid;
      grid-template-columns: repeat(4, 1fr);
      gap: 8px;
    }

    .metric-box {
      background-color: var(--bg-surface);
      border: none; /* Strokeless */
      border-radius: var(--radius-sub);
      padding: 12px 10px;
      text-align: center;
      display: flex;
      flex-direction: column;
      justify-content: center;
      gap: 4px;
      transition: background-color 0.2s ease;
    }

    .metric-box:hover {
      background-color: var(--bg-surface-hover);
    }

    .metric-label {
      font-size: 10px;
      font-weight: 700;
      color: var(--text-muted);
      text-transform: uppercase;
      letter-spacing: 0.03em;
    }

    .metric-value-row {
      display: flex;
      align-items: baseline;
      justify-content: center;
      gap: 3px;
    }

    .metric-number {
      font-size: 18px;
      font-weight: 700;
      color: var(--text-main);
      font-family: inherit;
      font-variant-numeric: tabular-nums;
      line-height: 1.1;
      letter-spacing: -0.02em;
    }

    .metric-unit {
      font-size: 10.5px;
      font-weight: 600;
      color: var(--text-muted);
    }

    /* Expression Matrix Card - M3 Expressive Morphing Buttons */
    .expr-grid {
      display: grid;
      grid-template-columns: repeat(4, 1fr);
      gap: 6px;
      margin-bottom: 8px;
    }

    .btn-expr {
      width: 100%;
      padding: 10px 4px;
      font-family: inherit;
      font-size: 11px;
      font-weight: 600;
      background-color: var(--bg-surface);
      border: none; /* Strokeless */
      outline: none;
      color: var(--text-muted);
      border-radius: 18px; /* Full Rounded Pill (proportional) */
      cursor: pointer;
      text-transform: uppercase;
      letter-spacing: 0.02em;
      transition: background-color 0.2s ease,
                  color 0.2s ease,
                  border-radius 0.25s cubic-bezier(0.2, 0, 0, 1),
                  transform 0.15s ease,
                  box-shadow 0.2s ease;
      display: flex;
      align-items: center;
      justify-content: center;
      user-select: none;
    }

    .btn-expr:hover {
      background-color: var(--bg-surface-hover);
      color: var(--text-main);
      border-radius: 12px;
    }

    .btn-expr:active {
      border-radius: 8px;
      transform: scale(0.96);
    }

    .btn-expr:focus, .btn-expr:focus-visible {
      outline: none;
    }

    .btn-expr.active {
      background-color: var(--accent-dark);
      color: #ffffff;
      border-radius: 12px;
      box-shadow: 0 2px 6px rgba(17, 24, 39, 0.22);
      font-weight: 700;
    }

    /* Material 3 Expressive Shape Library Morphing per Expression */
    .btn-expr[data-expr="0"]:hover, .btn-expr[data-expr="0"].active { border-radius: 12px; } /* IDLE: Balanced Squircle */
    .btn-expr[data-expr="1"]:hover, .btn-expr[data-expr="1"].active { border-radius: 16px 8px 16px 8px; } /* HAPPY: Clover / Flower */

    .btn-expr-auto {
      width: 100%;
      padding: 11px 16px;
      font-family: inherit;
      font-size: 12px;
      font-weight: 600;
      background-color: var(--bg-surface);
      border: none; /* Strokeless */
      outline: none;
      color: var(--text-main);
      border-radius: 22px; /* Full Rounded Pill (proportional) */
      cursor: pointer;
      transition: background-color 0.2s ease,
                  color 0.2s ease,
                  border-radius 0.25s cubic-bezier(0.2, 0, 0, 1),
                  transform 0.15s ease,
                  box-shadow 0.2s ease;
      display: flex;
      align-items: center;
      justify-content: center;
      user-select: none;
    }

    .btn-expr-auto:hover {
      background-color: var(--bg-surface-hover);
      border-radius: 12px; /* Morph to Medium on Hover */
    }

    .btn-expr-auto:active {
      border-radius: 9px;
      transform: scale(0.97);
    }

    .btn-expr-auto:focus, .btn-expr-auto:focus-visible {
      outline: none;
    }

    .btn-expr-auto.active {
      background-color: var(--accent-dark);
      color: #ffffff;
      border-radius: 12px; /* Morph to Medium when Selected */
      box-shadow: 0 2px 6px rgba(17, 24, 39, 0.22);
      font-weight: 700;
    }

    /* Network Configuration Panel - Strokeless Inner Fields */
    .network-container {
      width: 100%;
      max-width: 680px;
      margin: 0 auto;
    }

    .form-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 14px;
      margin-bottom: 14px;
    }

    .form-section {
      background-color: var(--bg-surface);
      border: none; /* Strokeless */
      border-radius: var(--radius-sub);
      padding: 14px;
    }

    .form-section-title {
      font-size: 12px;
      font-weight: 700;
      color: var(--text-main);
      margin-bottom: 10px;
    }

    .form-group {
      margin-bottom: 10px;
    }

    .form-group:last-child {
      margin-bottom: 0;
    }

    label {
      display: block;
      font-size: 10.5px;
      font-weight: 600;
      color: var(--text-muted);
      margin-bottom: 5px;
      text-transform: uppercase;
      letter-spacing: 0.02em;
    }

    input[type="text"], input[type="password"], input[type="number"] {
      width: 100%;
      padding: 10px 14px;
      font-family: inherit;
      font-size: 13px;
      border: 1px solid var(--border-card);
      border-radius: var(--radius-control);
      background-color: var(--bg-card);
      color: var(--text-main);
      outline: none;
      transition: border-color 0.2s ease, box-shadow 0.2s ease, background-color 0.2s ease;
      box-shadow: none;
    }

    input[type="text"]:focus, input[type="password"]:focus, input[type="number"]:focus {
      border-color: var(--accent-dark);
      box-shadow: 0 0 0 2px rgba(17, 24, 39, 0.15);
    }

    input::placeholder {
      color: var(--text-subtle);
    }

    .form-actions-row {
      display: flex;
      gap: 10px;
      align-items: center;
    }

    .btn-switch-mode {
      flex: 1;
      padding: 12px 18px;
      font-family: inherit;
      font-size: 12.5px;
      font-weight: 600;
      background-color: var(--bg-surface);
      border: none; /* Strokeless */
      outline: none;
      color: var(--text-main);
      border-radius: 22px; /* Full Rounded Pill (proportional) */
      cursor: pointer;
      transition: background-color 0.2s ease,
                  border-radius 0.25s cubic-bezier(0.2, 0, 0, 1),
                  transform 0.15s ease;
      text-align: center;
      user-select: none;
    }

    .btn-switch-mode:hover {
      background-color: var(--bg-surface-hover);
      border-radius: 12px; /* Morph to Medium on Hover */
    }

    .btn-switch-mode:active {
      border-radius: 9px;
      transform: scale(0.97);
    }

    .btn-switch-mode:focus, .btn-switch-mode:focus-visible {
      outline: none;
    }

    .btn-submit-main {
      flex: 1.2;
      padding: 12px 20px;
      background-color: var(--accent-dark);
      color: #ffffff;
      font-family: inherit;
      font-size: 12.5px;
      font-weight: 700;
      border: none; /* Strokeless */
      outline: none;
      border-radius: 22px; /* Full Rounded Pill (proportional) */
      cursor: pointer;
      box-shadow: 0 2px 6px rgba(17, 24, 39, 0.15);
      transition: background-color 0.2s ease,
                  border-radius 0.25s cubic-bezier(0.2, 0, 0, 1),
                  transform 0.15s ease,
                  box-shadow 0.2s ease;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 6px;
      user-select: none;
    }

    .btn-submit-main:hover {
      background-color: var(--accent-dark-hover);
      border-radius: 12px; /* Morph to Medium on Hover */
    }

    .btn-submit-main:active {
      border-radius: 9px;
      transform: scale(0.97);
    }

    .btn-submit-main:focus, .btn-submit-main:focus-visible {
      outline: none;
    }

    .btn-submit-main:disabled, .btn-switch-mode:disabled {
      opacity: 0.5;
      cursor: not-allowed;
      transform: none;
    }

    .btn-param, .btn-toggle {
      flex: 1;
      padding: 8px 4px;
      font-family: inherit;
      font-size: 11px;
      font-weight: 600;
      background-color: var(--bg-card);
      border: 1px solid var(--border-card);
      border-radius: var(--radius-control);
      color: var(--text-main);
      cursor: pointer;
      text-align: center;
      transition: background-color 0.2s ease, border-color 0.2s ease, color 0.2s ease;
    }

    .btn-param:hover, .btn-toggle:hover {
      background-color: var(--bg-surface-hover);
    }

    .btn-param.active, .btn-toggle.active {
      background-color: var(--accent-dark);
      color: #ffffff;
      border-color: var(--accent-dark);
    }

    /* Status Alert Box - Strokeless */
    #status {
      display: none;
      margin-top: 12px;
      padding: 12px 14px;
      font-size: 12px;
      font-weight: 600;
      border-radius: var(--radius-control);
      background-color: var(--bg-surface);
      color: var(--text-main);
      line-height: 1.5;
      border: none; /* Strokeless */
    }

    /* Tab Pane Visibility */
    .tab-pane {
      display: none;
      width: 100%;
    }

    .tab-pane.active {
      display: block;
    }

    /* Footer */
    .app-footer {
      width: 100%;
      text-align: center;
      padding: 6px 0 16px;
      font-size: 11px;
      color: var(--text-muted);
    }

    .app-footer a {
      color: var(--text-main);
      text-decoration: none;
      font-weight: 600;
      cursor: pointer;
    }

    .app-footer a:hover {
      text-decoration: underline;
    }

    .legal-modal {
      display: none;
      margin-top: 8px;
      padding: 12px;
      background-color: var(--bg-surface);
      border: none; /* Strokeless */
      border-radius: var(--radius-control);
      font-size: 10.5px;
      color: var(--text-muted);
      line-height: 1.6;
      text-align: left;
    }

    .tab-label-short { display: none; }
    .tab-label-full { display: inline; }

    /* Responsive Breakpoints */
    @media (max-width: 768px) {
      body {
        padding: 12px 10px 24px;
      }
      .app-wrapper {
        gap: 10px;
        width: 100%;
        max-width: 100%;
      }
      .tab-label-full { display: none; }
      .tab-label-short { display: inline; }

      .top-controls {
        display: flex;
        flex-direction: row;
        align-items: center;
        justify-content: space-between;
        gap: 8px;
        width: 100%;
      }
      .tab-segmented-control {
        flex: 1;
        min-width: 0;
        display: flex;
        padding: 3px;
        gap: 2px;
      }
      .tab-btn {
        flex: 1;
        min-width: 0;
        padding: 7px 4px 9px;
        font-size: 12px;
        gap: 4px;
        justify-content: center;
      }
      .mode-pill {
        flex-shrink: 0;
        padding: 6px 10px;
        font-size: 11px;
        font-weight: 700;
      }
      .bento-grid {
        display: flex;
        flex-direction: column;
        gap: 10px;
        width: 100%;
        align-items: stretch;
      }
      .bento-col {
        display: contents;
      }
      .bento-card {
        width: 100%;
        padding: 14px;
      }
      #card-gaze { order: 1; }
      #card-telemetry { order: 2; }
      #card-expression { order: 3; }
      #card-display-ambient { order: 4; }
      #card-cognition { order: 5; }

      .metrics-grid {
        grid-template-columns: repeat(2, 1fr);
        gap: 6px;
      }
      .metric-box {
        padding: 10px 8px;
      }
      .metric-number {
        font-size: 16px;
      }
      .expr-grid {
        grid-template-columns: repeat(4, 1fr);
        gap: 5px;
      }
      .btn-expr {
        padding: 8px 2px;
        font-size: 10px;
      }
      .btn-param, .btn-toggle {
        flex: 1;
        min-width: 0;
        padding: 8px 2px;
        font-size: 11px;
      }
      .form-grid {
        grid-template-columns: 1fr;
        gap: 10px;
      }
      .form-actions-row {
        flex-direction: column;
      }
      .btn-switch-mode, .btn-submit-main {
        width: 100%;
      }
    }
  </style>
</head>
<body>
  <div class="app-wrapper">
    
    <!-- Top Navigation Controls -->
    <div class="top-controls">
      <div class="tab-segmented-control">
        <button type="button" class="tab-btn active" data-target="tab-control">
          <svg class="tab-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <circle cx="12" cy="12" r="10"></circle>
            <circle cx="12" cy="12" r="3"></circle>
          </svg>
          <span class="tab-label-full">Control & Rig</span>
          <span class="tab-label-short">Control</span>
        </button>
        <button type="button" class="tab-btn" data-target="tab-network">
          <svg class="tab-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M5 12.55a11 11 0 0 1 14.08 0"></path>
            <path d="M1.42 9a16 16 0 0 1 21.16 0"></path>
            <path d="M8.53 16.11a6 6 0 0 1 6.95 0"></path>
            <line x1="12" y1="20" x2="12.01" y2="20"></line>
          </svg>
          <span class="tab-label-full">Network</span>
          <span class="tab-label-short">Network</span>
        </button>
        <button type="button" class="tab-btn" data-target="tab-device">
          <svg class="tab-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
            <path d="M17.5 19H9a7 7 0 1 1 6.71-9h1.79a4.5 4.5 0 1 1 0 9Z"></path>
          </svg>
          <span class="tab-label-full">Weather & Location</span>
          <span class="tab-label-short">Weather</span>
        </button>
      </div>
      <div id="mode-badge" class="mode-pill">STA Online</div>
    </div>

    <!-- Tab 1: Control & Telemetry Bento Grid -->
    <div id="tab-control" class="tab-pane active">
      <div class="bento-grid">
        
        <!-- Left Bento Column: Gaze Steering & Display Ambience -->
        <div class="bento-col">
          
          <!-- Bento Card 1: Gaze Steering Pad -->
          <section class="bento-card" id="card-gaze">
            <div class="bento-card-header">
              <h2 class="card-title">Gaze Steering Pad</h2>
              <div style="display:flex;align-items:center;gap:6px;">
                <span class="card-badge" id="gaze-coords" style="font-family:monospace;font-variant-numeric:tabular-nums;font-weight:700;">X: 0.00 | Y: 0.00</span>
                <button type="button" id="btn-center-gaze" class="btn-reset-gaze" title="Reset Gaze to Center">⌖ Center</button>
              </div>
            </div>
            <div class="gaze-viewport" id="gaze-viewport">
              <canvas id="gaze-canvas"></canvas>
            </div>
            <div class="gaze-hint">
              <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><path d="M12 8v8"/><path d="M8 12h8"/></svg>
              <span>Click or drag to steer LoRe's attention in real time</span>
            </div>
          </section>

          <!-- Bento Card 2: Display & Ambience -->
          <section class="bento-card" id="card-display-ambient">
            <div class="bento-card-header">
              <h2 class="card-title">Display & Ambience</h2>
              <div style="display:flex;align-items:center;gap:6px;">
                <span class="card-badge" id="bright-badge">128 (50%)</span>
                <button type="button" id="btn-reset-brightness" class="btn-param" style="padding:2px 8px;font-size:10px;border-radius:10px;height:22px;line-height:1;" title="Reset to Default (50%)">↺ Reset</button>
              </div>
            </div>
            <div class="form-section" style="padding:10px 12px;margin-bottom:0;">
              <div class="form-group" style="margin-bottom:6px;">
                <label>OLED Brightness</label>
                <input type="range" id="oled-brightness-slider" min="0" max="255" value="128" style="width:100%;cursor:pointer;accent-color:var(--accent-dark);">
              </div>
              <div style="display:flex;justify-content:space-between;font-size:10px;color:var(--text-muted);margin-bottom:12px;">
                <span>0% (Off)</span>
                <span>50% (Default)</span>
                <span>100% (Max)</span>
              </div>
              <div class="form-group" style="margin-bottom:8px;">
                <label>Smart Auto-Luminance</label>
                <div style="display:flex;gap:6px;">
                  <button type="button" id="btn-auto-bright-toggle" class="btn-toggle" style="padding:6px 12px;font-size:11px;">Auto-Brightness: OFF</button>
                </div>
              </div>
              <div class="form-group" style="margin-bottom:0;">
                <label>Test Glance Popups</label>
                <div style="display:flex;gap:6px;">
                  <button type="button" id="btn-trigger-clock" class="btn-param" style="padding:6px 10px;font-size:11px;">Clock Glance</button>
                  <button type="button" id="btn-trigger-weather" class="btn-param" style="padding:6px 10px;font-size:11px;">Weather Glance</button>
                </div>
              </div>
            </div>
          </section>

        </div>

        <!-- Right Bento Column: Telemetry, Expression, Cognition -->
        <div class="bento-col">
          
          <!-- Bento Card 1: AI Telemetry -->
          <section class="bento-card" id="card-telemetry">
            <div class="bento-card-header">
              <h2 class="card-title">Telemetry</h2>
            </div>
            
            <div class="metrics-grid">
              <div class="metric-box">
                <span class="metric-label">Inference</span>
                <div class="metric-value-row">
                  <span id="tel-fps" class="metric-number">0.0</span>
                  <span class="metric-unit">FPS</span>
                </div>
              </div>

              <div class="metric-box">
                <span class="metric-label">Confidence</span>
                <div class="metric-value-row">
                  <span id="tel-conf" class="metric-number">0.00</span>
                </div>
              </div>

              <div class="metric-box">
                <span class="metric-label">Human</span>
                <div class="metric-value-row">
                  <span id="tel-human" class="metric-number">0.00</span>
                </div>
              </div>

              <div class="metric-box">
                <span class="metric-label">Proximity</span>
                <div class="metric-value-row">
                  <span id="tel-prox" class="metric-number">0%</span>
                </div>
              </div>
            </div>
          </section>

          <!-- Bento Card 2: Facial Expression Rig -->
          <section class="bento-card" id="card-expression">
            <div class="bento-card-header">
              <h2 class="card-title">Expression</h2>
            </div>

            <div class="expr-grid">
              <button type="button" class="btn-expr" data-expr="0">IDLE</button>
              <button type="button" class="btn-expr" data-expr="1">HAPPY</button>
            </div>

            <button type="button" id="btn-expr-auto" class="btn-expr-auto active">Default (Auto Mood)</button>
          </section>

          <!-- Bento Card 3: Cognitive & Neural State -->
          <section class="bento-card" id="card-cognition">
            <div class="bento-card-header">
              <h2 class="card-title">Cognitive & Neural State</h2>
            </div>
            <div class="form-section" style="padding:10px 12px;margin-bottom:0;">
              <div style="margin-bottom:10px;">
                <div style="font-size:10px;font-weight:700;color:var(--text-muted);text-transform:uppercase;letter-spacing:0.03em;margin-bottom:4px;">Inner Monologue</div>
                <div id="tel-thought" style="font-size:12px;font-style:italic;color:var(--text-main);background:var(--bg-surface);padding:8px 10px;border-radius:var(--radius-sub);min-height:32px;display:flex;align-items:center;">"Pondering surrounding reality..."</div>
              </div>
              <div class="metrics-grid" style="grid-template-columns: repeat(3, 1fr);gap:6px;">
                <div class="metric-box" style="padding:8px 6px;">
                  <span class="metric-label">Bonding</span>
                  <div class="metric-value-row">
                    <span id="tel-bonding" class="metric-number" style="font-size:15px;">0%</span>
                  </div>
                </div>
                <div class="metric-box" style="padding:8px 6px;">
                  <span class="metric-label">Energy</span>
                  <div class="metric-value-row">
                    <span id="tel-energy" class="metric-number" style="font-size:15px;">100%</span>
                  </div>
                </div>
                <div class="metric-box" style="padding:8px 6px;">
                  <span class="metric-label">Curiosity</span>
                  <div class="metric-value-row">
                    <span id="tel-curiosity" class="metric-number" style="font-size:15px;">0%</span>
                  </div>
                </div>
              </div>
            </div>
          </section>

        </div>

      </div>
    </div>

    <!-- Tab 2: Network Configuration -->
    <div id="tab-network" class="tab-pane">
      <div class="network-container">
        <section class="bento-card">
          <div class="bento-card-header">
            <h2 class="card-title">Network Configuration</h2>
          </div>

          <form id="wifi-form">
            <div class="form-grid">
              
              <!-- STA Column -->
              <div class="form-section">
                <div class="form-section-title">Wi-Fi Client (STA Mode)</div>
                <div class="form-group">
                  <label for="sta_ssid">SSID</label>
                  <input type="text" id="sta_ssid" name="sta_ssid" placeholder="Wi-Fi Router SSID" required>
                </div>
                <div class="form-group">
                  <label for="sta_pass">Password</label>
                  <input type="password" id="sta_pass" name="sta_pass" placeholder="Wi-Fi Password">
                </div>
              </div>

              <!-- AP Column -->
              <div class="form-section">
                <div class="form-section-title">Access Point (AP Mode)</div>
                <div class="form-group">
                  <label for="ap_ssid">SSID</label>
                  <input type="text" id="ap_ssid" name="ap_ssid" placeholder="LoRe AP SSID" required>
                </div>
                <div class="form-group">
                  <label for="ap_pass">Password</label>
                  <input type="password" id="ap_pass" name="ap_pass" placeholder="Min. 8 characters or empty">
                </div>
              </div>

              <!-- BLE Column -->
              <div class="form-section">
                <div class="form-section-title">Bluetooth Low Energy (BLE)</div>
                <div class="form-group">
                  <label for="ble_name">Device Name</label>
                  <input type="text" id="ble_name" name="ble_name" placeholder="LoRe-Sense" required>
                </div>
              </div>

            </div>

            <div class="form-actions-row">
              <button type="button" id="btn-switch-mode" class="btn-switch-mode" style="display:none;"></button>
              <button type="submit" id="btn-save" class="btn-submit-main">Save & Connect →</button>
            </div>
          </form>

          <div id="status"></div>
        </section>
      </div>
    </div>

    <!-- Tab 3: Device & OTA Settings -->
    <div id="tab-device" class="tab-pane">
      <div class="network-container">
        
        <!-- Weather Configuration Card (Open-Meteo) -->
        <section class="bento-card" style="margin-bottom:14px;">
          <div class="bento-card-header">
            <h2 class="card-title">Weather & Location</h2>
          </div>

          <!-- Live City Search Section (Full Width & Outlined) -->
          <div style="margin-bottom:14px;">
            <div class="form-group" style="position:relative;margin-bottom:0;">
              <label for="weather-search-input" style="font-size:11px;font-weight:700;color:var(--text-main);margin-bottom:6px;display:flex;align-items:center;gap:6px;">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="11" cy="11" r="8"></circle><line x1="21" y1="21" x2="16.65" y2="16.65"></line></svg>
                <span>Search City</span>
              </label>
              <div style="position:relative;">
                <input type="text" id="weather-search-input" placeholder="Type city or location name... (e.g. London, Tokyo, New York, Paris, Jakarta)" style="width:100%;padding:12px 14px 12px 38px;font-size:13px;border:1.5px solid var(--border-card);border-radius:var(--radius-control);background:var(--bg-card);color:var(--text-main);" autocomplete="off">
                <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="position:absolute;left:13px;top:50%;transform:translateY(-50%);color:var(--text-muted);pointer-events:none;"><circle cx="11" cy="11" r="8"></circle><line x1="21" y1="21" x2="16.65" y2="16.65"></line></svg>
              </div>
              <div id="weather-search-results" style="display:none;position:absolute;top:100%;left:0;right:0;background:var(--bg-card);border:1px solid var(--border-card);border-radius:var(--radius-control);z-index:99;max-height:200px;overflow-y:auto;box-shadow:0 8px 24px rgba(0,0,0,0.18);margin-top:4px;"></div>
            </div>
          </div>

          <div class="form-grid">
            <div class="form-section">
              <div class="form-section-title">Preset / Manual Selection</div>
              <div class="form-group">
                <label for="weather-preset-select">City Preset List</label>
                <select id="weather-preset-select" style="width:100%;padding:10px 12px;font-family:inherit;font-size:12px;border:1px solid var(--border-card);border-radius:var(--radius-control);background:var(--bg-card);color:var(--text-main);">
                  <option value="custom">-- Custom / Search Result / GPS --</option>
                  <optgroup label="Indonesia - Java">
                    <option value="jakarta">Jakarta (-6.2088, 106.8456)</option>
                    <option value="bandung">Bandung (-6.9175, 107.6191)</option>
                    <option value="surabaya">Surabaya (-7.2575, 112.7521)</option>
                    <option value="semarang">Semarang (-6.9667, 110.4167)</option>
                    <option value="yogyakarta">Yogyakarta (-7.7956, 110.3695)</option>
                    <option value="solo">Surakarta / Solo (-7.5666, 110.8167)</option>
                    <option value="malang">Malang (-7.9797, 112.6304)</option>
                    <option value="bogor">Bogor (-6.5950, 106.8166)</option>
                    <option value="depok">Depok (-6.4025, 106.7942)</option>
                    <option value="tangerang">Tangerang (-6.1783, 106.6319)</option>
                    <option value="bekasi">Bekasi (-6.2383, 106.9756)</option>
                  </optgroup>
                  <optgroup label="Indonesia - Sumatra">
                    <option value="medan">Medan (3.5952, 98.6722)</option>
                    <option value="palembang">Palembang (-2.9761, 104.7754)</option>
                    <option value="padang">Padang (-0.9471, 100.4172)</option>
                    <option value="pekanbaru">Pekanbaru (0.5071, 101.4478)</option>
                    <option value="lampung">Bandar Lampung (-5.4500, 105.2667)</option>
                    <option value="batam">Batam (1.1301, 104.0529)</option>
                    <option value="aceh">Banda Aceh (5.5483, 95.3238)</option>
                  </optgroup>
                  <optgroup label="Indonesia - Bali & Nusa Tenggara">
                    <option value="bali">Denpasar / Bali (-8.6705, 115.2126)</option>
                    <option value="mataram">Mataram / Lombok (-8.5833, 116.1167)</option>
                    <option value="kupang">Kupang (-10.1772, 123.6070)</option>
                  </optgroup>
                  <optgroup label="Indonesia - Kalimantan & IKN">
                    <option value="ikn">IKN Nusantara (-0.9744, 116.7027)</option>
                    <option value="balikpapan">Balikpapan (-1.2379, 116.8289)</option>
                    <option value="samarinda">Samarinda (-0.5022, 117.1536)</option>
                    <option value="banjarmasin">Banjarmasin (-3.3194, 114.5908)</option>
                    <option value="pontianak">Pontianak (-0.0263, 109.3425)</option>
                  </optgroup>
                  <optgroup label="Indonesia - Sulawesi & East">
                    <option value="makassar">Makassar (-5.1477, 119.4327)</option>
                    <option value="manado">Manado (1.4748, 124.8428)</option>
                    <option value="jayapura">Jayapura (-2.5337, 140.7181)</option>
                    <option value="ambon">Ambon (-3.6954, 128.1814)</option>
                  </optgroup>
                  <optgroup label="International">
                    <option value="singapore">Singapore (1.3521, 103.8198)</option>
                    <option value="kualalumpur">Kuala Lumpur (3.1390, 101.6869)</option>
                    <option value="tokyo">Tokyo (35.6762, 139.6503)</option>
                    <option value="seoul">Seoul (37.5665, 126.9780)</option>
                    <option value="london">London (51.5074, -0.1278)</option>
                    <option value="newyork">New York (40.7128, -74.0060)</option>
                    <option value="paris">Paris (48.8566, 2.3522)</option>
                    <option value="dubai">Dubai (25.2048, 55.2708)</option>
                    <option value="sydney">Sydney (-33.8688, 151.2093)</option>
                  </optgroup>
                </select>
              </div>
              <div class="form-group">
                <label for="weather-city-input">OLED Display Label</label>
                <input type="text" id="weather-city-input" placeholder="e.g. Jakarta" value="Jakarta" required maxlength="20">
              </div>
              <div class="form-group">
                <label for="weather-tz-select">Timezone (UTC Offset)</label>
                <select id="weather-tz-select" style="width:100%;padding:10px 12px;font-family:inherit;font-size:12px;border:1px solid var(--border-card);border-radius:var(--radius-control);background:var(--bg-card);color:var(--text-main);">
                  <option value="25200" selected>UTC+07:00 (WIB - Jakarta, Bangkok, Hanoi)</option>
                  <option value="28800">UTC+08:00 (WITA - Bali, Singapore, Taipei, Perth)</option>
                  <option value="32400">UTC+09:00 (WIT - Tokyo, Seoul, Jayapura)</option>
                  <option value="0">UTC+00:00 (GMT/UTC - London, Dublin, Lisbon)</option>
                  <option value="3600">UTC+01:00 (CET - Paris, Berlin, Rome, Madrid)</option>
                  <option value="7200">UTC+02:00 (EET - Athens, Cairo, Helsinki, Kyiv)</option>
                  <option value="10800">UTC+03:00 (MSK/AST - Moscow, Riyadh, Istanbul)</option>
                  <option value="14400">UTC+04:00 (GST - Dubai, Abu Dhabi, Baku)</option>
                  <option value="19800">UTC+05:30 (IST - New Delhi, Mumbai, Colombo)</option>
                  <option value="20700">UTC+05:45 (NPT - Kathmandu)</option>
                  <option value="21600">UTC+06:00 (BST - Dhaka, Almaty)</option>
                  <option value="36000">UTC+10:00 (AEST - Sydney, Melbourne, Brisbane)</option>
                  <option value="39600">UTC+11:00 (SBT - Noumea, Solomon Is.)</option>
                  <option value="43200">UTC+12:00 (NZST - Auckland, Suva)</option>
                  <option value="-36000">UTC-10:00 (HST - Honolulu, Hawaii)</option>
                  <option value="-32400">UTC-09:00 (AKST - Anchorage, Alaska)</option>
                  <option value="-28800">UTC-08:00 (PST - Los Angeles, San Francisco, Vancouver)</option>
                  <option value="-25200">UTC-07:00 (MST - Denver, Phoenix, Calgary)</option>
                  <option value="-21600">UTC-06:00 (CST - Chicago, Dallas, Mexico City)</option>
                  <option value="-18000">UTC-05:00 (EST - New York, Washington, Toronto, Miami)</option>
                  <option value="-14400">UTC-04:00 (AST - Santiago, Halifax, Manaus)</option>
                  <option value="-10800">UTC-03:00 (BRT/ART - São Paulo, Buenos Aires)</option>
                  <option value="-3600">UTC-01:00 (CVT - Cape Verde, Azores)</option>
                </select>
              </div>
            </div>

            <div class="form-section">
              <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:10px;">
                <div class="form-section-title" style="margin-bottom:0;">Geographical Coordinates</div>
                <button type="button" id="btn-gps-location" class="btn-param" style="flex:none;width:auto;padding:4px 10px;font-size:11px;border-radius:12px;height:26px;display:flex;align-items:center;gap:5px;" title="Detect GPS Coordinates">
                  <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 10c0 7-9 13-9 13s-9-6-9-13a9 9 0 0 1 18 0z"></path><circle cx="12" cy="10" r="3"></circle></svg>
                  <span>My GPS</span>
                </button>
              </div>
              <div class="form-group">
                <label for="weather-lat-input">Latitude</label>
                <input type="number" step="0.0001" id="weather-lat-input" placeholder="-6.2088" value="-6.2088" required>
              </div>
              <div class="form-group">
                <label for="weather-lon-input">Longitude</label>
                <input type="number" step="0.0001" id="weather-lon-input" placeholder="106.8456" value="106.8456" required>
              </div>
            </div>
          </div>

          <div style="margin-top:12px;display:flex;align-items:center;gap:8px;">
            <input type="checkbox" id="weather-enable-check" checked style="accent-color:var(--accent-dark);width:16px;height:16px;cursor:pointer;">
            <label for="weather-enable-check" style="font-size:12px;font-weight:600;cursor:pointer;margin:0;">Show Spontaneous Weather & Clock Glances during Idle Standby</label>
          </div>

          <div class="form-actions-row" style="margin-top:14px;display:flex;gap:8px;flex-wrap:wrap;">
            <button type="button" id="btn-preview-clock" class="btn-switch-mode" style="flex:1;min-width:110px;">Preview Clock</button>
            <button type="button" id="btn-preview-weather" class="btn-switch-mode" style="flex:1;min-width:110px;">Preview Weather</button>
            <button type="button" id="btn-save-weather" class="btn-submit-main" style="flex:1.4;min-width:130px;">Save & Fetch →</button>
          </div>

          <div id="weather-live-info" style="margin-top:10px;padding:10px 12px;font-size:11.5px;background:var(--bg-surface);border-radius:var(--radius-control);color:var(--text-muted);display:none;"></div>
        </section>

        <!-- Phone Notifications (Ntfy.sh Cloud Push & Local Webhook) -->
        <section class="bento-card" style="margin-bottom:14px;">
          <div class="bento-card-header">
            <h2 class="card-title">Phone Notifications (ntfy.sh & Webhook)</h2>
            <span class="card-badge" id="ntfy-status-badge">ntfy: idle</span>
          </div>

          <div class="form-section" style="padding:10px 12px;">
            <div class="form-group">
              <label for="ntfy-topic-input">Ntfy.sh Topic Name (Unique / Private)</label>
              <div style="display:flex;gap:6px;">
                <input type="text" id="ntfy-topic-input" placeholder="e.g. lore_notif_myphone123" value="lore_notif_default" style="flex:1;">
                <button type="button" id="btn-save-ntfy" class="btn-toggle active" style="flex:none;padding:0 14px;border-radius:10px;">Save Topic</button>
              </div>
            </div>

            <div style="margin-top:10px;display:flex;gap:8px;flex-wrap:wrap;">
              <button type="button" id="btn-test-notif" class="btn-switch-mode" style="flex:1;min-width:140px;">Send Test Notification</button>
            </div>

            <div style="margin-top:12px;padding:10px 12px;background:var(--bg-surface);border-radius:var(--radius-control);font-size:11px;color:var(--text-muted);line-height:1.5;">
              <b style="color:var(--text-main);">Cara Hubungkan ke HP (Android / iPhone):</b><br>
              1. <b>Metode MacroDroid / Tasker:</b> Buat Trigger <i>"Notification Received"</i> (WhatsApp/Telegram), lalu Action <i>"HTTP Request POST"</i> ke <code style="background:var(--bg-card);padding:2px 4px;border-radius:4px;" id="code-ntfy-url">https://ntfy.sh/lore_notif_default</code> dengan Header <code style="background:var(--bg-card);padding:2px 4px;border-radius:4px;">Title: [notif_title]</code> dan Body <code style="background:var(--bg-card);padding:2px 4px;border-radius:4px;">[notif_body]</code>.<br>
              2. <b>Metode App ntfy:</b> Download app <b>ntfy</b> di Play Store/App Store, lalu publish pesan ke topik Anda.<br>
              3. <b>Metode Webhook Lokal:</b> POST ke <code style="background:var(--bg-card);padding:2px 4px;border-radius:4px;" id="code-local-url">http://lore.local/api/notify</code> (JSON: <code style="background:var(--bg-card);padding:2px 4px;border-radius:4px;">{"sender":"Budi","message":"Halo"}</code>).<br>
              4. <b>Metode Bluetooth BLE (Offline / Outdoor):</b> Kirim teks notifikasi langsung ke Bluetooth <b>LoRe-Sense</b> via Plugin Serial Bluetooth di MacroDroid (format: <code style="background:var(--bg-card);padding:2px 4px;border-radius:4px;">[WA] {not_title}: {not_body}</code>).
            </div>
          </div>
        </section>

      </div>
    </div>

    <!-- Footer Disclosures -->
    <footer class="app-footer">
      <p>LoRe Biomechanical Engine &bull; On-Device Local Processing</p>
      <p style="margin-top:4px;"><a id="toggle-privacy">Privacy Policy</a> &bull; <a id="toggle-terms">Terms of Service</a></p>
      <div id="legal-content" class="legal-modal">
        <strong>Terms of Service & Privacy Policy:</strong><br>
        1. Data Processing: All affective cognition, minimum-jerk kinematics, and oculomotor dynamics are executed locally in ESP32-S3 internal SRAM.<br>
        2. Privacy: No private sensor data or Wi-Fi credentials are sent to external third-party cloud servers.<br>
        3. Credentials: Access Point and STA credentials are stored securely in local device NVS Flash.
      </div>
    </footer>

  </div>

  <script>
    /* Gaze Steering Pad & Radar Setup */
    const gazeCanvas = document.getElementById('gaze-canvas');
    const gazeCtx = gazeCanvas ? gazeCanvas.getContext('2d') : null;
    const gazeViewport = document.getElementById('gaze-viewport');
    const gazeCoordsBadge = document.getElementById('gaze-coords');

    let userGazeTarget = { x: 0, y: 0, active: false, activeUntil: 0 };
    let robotGazeState = { x: 0, y: 0, detected: false };
    let gazeTrail = [];

    function resizeGazeCanvas() {
      if (!gazeViewport || !gazeCanvas) return;
      const rect = gazeViewport.getBoundingClientRect();
      const dpr = window.devicePixelRatio || 1;
      const w = Math.round(rect.width);
      const h = Math.round(rect.height);
      if (w > 0 && h > 0) {
        if (gazeCanvas.width !== w * dpr || gazeCanvas.height !== h * dpr) {
          gazeCanvas.width = w * dpr;
          gazeCanvas.height = h * dpr;
        }
      }
      renderGazeRadar();
    }
    window.addEventListener('resize', resizeGazeCanvas);

    function renderGazeRadar() {
      if (!gazeCanvas || !gazeCtx) return;
      const dpr = window.devicePixelRatio || 1;
      const w = gazeCanvas.width / dpr;
      const h = gazeCanvas.height / dpr;
      if (w <= 0 || h <= 0) return;

      gazeCtx.save();
      gazeCtx.scale(dpr, dpr);
      gazeCtx.clearRect(0, 0, w, h);

      const cx = w / 2;
      const cy = h / 2;
      const maxR = Math.min(w, h) * 0.44;

      // 1. Concentric Range Rings (25%, 50%, 75%, 100%) - Subtle Monochrome
      const rings = [0.25, 0.50, 0.75, 1.0];
      rings.forEach((pct, idx) => {
        gazeCtx.beginPath();
        gazeCtx.arc(cx, cy, maxR * pct, 0, Math.PI * 2);
        if (idx === 3) {
          gazeCtx.strokeStyle = 'rgba(25, 28, 32, 0.26)';
          gazeCtx.lineWidth = 1.5;
          gazeCtx.setLineDash([]);
        } else {
          gazeCtx.strokeStyle = 'rgba(25, 28, 32, 0.09)';
          gazeCtx.lineWidth = 1.0;
          gazeCtx.setLineDash([3, 4]);
        }
        gazeCtx.stroke();
      });

      // 2. Crosshair Axes & Sub-Ticks
      gazeCtx.setLineDash([4, 4]);
      gazeCtx.strokeStyle = 'rgba(25, 28, 32, 0.14)';
      gazeCtx.lineWidth = 1.0;
      gazeCtx.beginPath();
      gazeCtx.moveTo(cx - maxR, cy);
      gazeCtx.lineTo(cx + maxR, cy);
      gazeCtx.moveTo(cx, cy - maxR);
      gazeCtx.lineTo(cx, cy + maxR);
      gazeCtx.stroke();

      // Half-radius sub-ticks
      gazeCtx.setLineDash([]);
      gazeCtx.strokeStyle = 'rgba(25, 28, 32, 0.20)';
      gazeCtx.lineWidth = 1.0;
      gazeCtx.beginPath();
      const rHalf = maxR * 0.5;
      gazeCtx.moveTo(cx - 3, cy - rHalf); gazeCtx.lineTo(cx + 3, cy - rHalf);
      gazeCtx.moveTo(cx - 3, cy + rHalf); gazeCtx.lineTo(cx + 3, cy + rHalf);
      gazeCtx.moveTo(cx - rHalf, cy - 3); gazeCtx.lineTo(cx - rHalf, cy + 3);
      gazeCtx.moveTo(cx + rHalf, cy - 3); gazeCtx.lineTo(cx + rHalf, cy + 3);
      gazeCtx.stroke();

      // 3. Coordinate Labels
      gazeCtx.fillStyle = 'rgba(25, 28, 32, 0.42)';
      gazeCtx.font = '700 8.5px "Plus Jakarta Sans", monospace, sans-serif';
      gazeCtx.textAlign = 'center';
      gazeCtx.textBaseline = 'bottom';
      gazeCtx.fillText('UP (-1.0)', cx, cy - maxR - 4);
      gazeCtx.textBaseline = 'top';
      gazeCtx.fillText('DOWN (+1.0)', cx, cy + maxR + 4);
      gazeCtx.textAlign = 'right';
      gazeCtx.textBaseline = 'middle';
      gazeCtx.fillText('LEFT', cx - maxR - 6, cy);
      gazeCtx.textAlign = 'left';
      gazeCtx.fillText('RIGHT', cx + maxR + 6, cy);

      // 4. Center Anchor Dot
      gazeCtx.beginPath();
      gazeCtx.arc(cx, cy, 2.5, 0, Math.PI * 2);
      gazeCtx.fillStyle = 'rgba(25, 28, 32, 0.35)';
      gazeCtx.fill();

      // 5. Gaze Motion Trail
      if (gazeTrail.length > 1) {
        gazeCtx.beginPath();
        gazeCtx.moveTo(cx + gazeTrail[0].x * maxR, cy + gazeTrail[0].y * maxR);
        for (let i = 1; i < gazeTrail.length; i++) {
          gazeCtx.lineTo(cx + gazeTrail[i].x * maxR, cy + gazeTrail[i].y * maxR);
        }
        gazeCtx.strokeStyle = 'rgba(25, 28, 32, 0.18)';
        gazeCtx.lineWidth = 1.5;
        gazeCtx.setLineDash([3, 3]);
        gazeCtx.stroke();
        gazeCtx.setLineDash([]);
      }

      // 6. Robot Autonomous Gaze Focus Reticle (Solid Ink Black)
      const robotScreenX = cx + robotGazeState.x * maxR;
      const robotScreenY = cy + robotGazeState.y * maxR;

      // Soft radial shadow halo
      const grad = gazeCtx.createRadialGradient(robotScreenX, robotScreenY, 2, robotScreenX, robotScreenY, 18);
      grad.addColorStop(0, 'rgba(25, 28, 32, 0.10)');
      grad.addColorStop(0.6, 'rgba(25, 28, 32, 0.03)');
      grad.addColorStop(1, 'rgba(25, 28, 32, 0)');
      gazeCtx.fillStyle = grad;
      gazeCtx.beginPath();
      gazeCtx.arc(robotScreenX, robotScreenY, 18, 0, Math.PI * 2);
      gazeCtx.fill();

      // Outer reticle ring
      gazeCtx.strokeStyle = '#191c20';
      gazeCtx.lineWidth = 2;
      gazeCtx.beginPath();
      gazeCtx.arc(robotScreenX, robotScreenY, 8, 0, Math.PI * 2);
      gazeCtx.stroke();

      // Inner pupil dot with white center
      gazeCtx.fillStyle = '#191c20';
      gazeCtx.beginPath();
      gazeCtx.arc(robotScreenX, robotScreenY, 4, 0, Math.PI * 2);
      gazeCtx.fill();

      gazeCtx.fillStyle = '#ffffff';
      gazeCtx.beginPath();
      gazeCtx.arc(robotScreenX, robotScreenY, 1.5, 0, Math.PI * 2);
      gazeCtx.fill();

      // Reticle Tag
      gazeCtx.font = '700 9px "Plus Jakarta Sans", monospace, sans-serif';
      gazeCtx.fillStyle = '#191c20';
      gazeCtx.textAlign = 'left';
      gazeCtx.textBaseline = 'bottom';
      gazeCtx.fillText('GAZE', robotScreenX + 11, robotScreenY - 2);

      // 7. Interactive User Target Pointer (Monochrome)
      const now = Date.now();
      if (userGazeTarget.active && now < userGazeTarget.activeUntil) {
        const uScreenX = cx + userGazeTarget.x * maxR;
        const uScreenY = cy + userGazeTarget.y * maxR;

        // Dashed targeting ring
        gazeCtx.strokeStyle = '#191c20';
        gazeCtx.lineWidth = 1.5;
        gazeCtx.setLineDash([4, 3]);
        gazeCtx.beginPath();
        gazeCtx.arc(uScreenX, uScreenY, 13, 0, Math.PI * 2);
        gazeCtx.stroke();
        gazeCtx.setLineDash([]);

        // Crosshair ticks
        gazeCtx.strokeStyle = '#191c20';
        gazeCtx.lineWidth = 1.5;
        gazeCtx.beginPath();
        gazeCtx.moveTo(uScreenX - 17, uScreenY);
        gazeCtx.lineTo(uScreenX - 6, uScreenY);
        gazeCtx.moveTo(uScreenX + 6, uScreenY);
        gazeCtx.lineTo(uScreenX + 17, uScreenY);
        gazeCtx.moveTo(uScreenX, uScreenY - 17);
        gazeCtx.lineTo(uScreenX, uScreenY - 6);
        gazeCtx.moveTo(uScreenX, uScreenY + 6);
        gazeCtx.lineTo(uScreenX, uScreenY + 17);
        gazeCtx.stroke();

        // Target center point
        gazeCtx.fillStyle = '#191c20';
        gazeCtx.beginPath();
        gazeCtx.arc(uScreenX, uScreenY, 2, 0, Math.PI * 2);
        gazeCtx.fill();

        // Monochrome Pill Badge
        const tagX = uScreenX + 9;
        const tagY = uScreenY + 7;
        const tagW = 42;
        const tagH = 15;
        const tagR = 4;
        gazeCtx.fillStyle = '#191c20';
        gazeCtx.beginPath();
        if (typeof gazeCtx.roundRect === 'function') {
          gazeCtx.roundRect(tagX, tagY, tagW, tagH, tagR);
        } else {
          gazeCtx.rect(tagX, tagY, tagW, tagH);
        }
        gazeCtx.fill();

        gazeCtx.fillStyle = '#ffffff';
        gazeCtx.font = '700 8.5px "Plus Jakarta Sans", monospace, sans-serif';
        gazeCtx.textAlign = 'center';
        gazeCtx.textBaseline = 'middle';
        gazeCtx.fillText('TARGET', tagX + tagW / 2, tagY + tagH / 2);
      }

      gazeCtx.restore();
    }

    let isGazeDragging = false;
    let lastGazeSendMs = 0;

    function handleGazeInteraction(clientX, clientY, persistMs = 3500) {
      if (!gazeViewport) return;
      const rect = gazeViewport.getBoundingClientRect();
      const normX = Math.max(-1.0, Math.min(1.0, ((clientX - rect.left) / rect.width) * 2.0 - 1.0));
      const normY = Math.max(-1.0, Math.min(1.0, ((clientY - rect.top) / rect.height) * 2.0 - 1.0));

      userGazeTarget.x = normX;
      userGazeTarget.y = normY;
      userGazeTarget.active = true;
      userGazeTarget.activeUntil = Date.now() + persistMs;

      if (gazeCoordsBadge) {
        const sx = normX >= 0 ? '+' : '';
        const sy = normY >= 0 ? '+' : '';
        gazeCoordsBadge.innerText = `X: ${sx}${normX.toFixed(2)} | Y: ${sy}${normY.toFixed(2)}`;
      }

      renderGazeRadar();

      const now = Date.now();
      if (now - lastGazeSendMs > 50) {
        lastGazeSendMs = now;
        fetch('/set_gaze', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ x: normX, y: normY, duration_ms: persistMs })
        }).catch(() => {});
      }
    }

    if (gazeViewport) {
      gazeViewport.addEventListener('pointerdown', e => {
        isGazeDragging = true;
        try { gazeViewport.setPointerCapture(e.pointerId); } catch (_) {}
        handleGazeInteraction(e.clientX, e.clientY);
      });
      gazeViewport.addEventListener('pointermove', e => {
        if (isGazeDragging) {
          handleGazeInteraction(e.clientX, e.clientY);
        }
      });
      const endGaze = e => {
        if (isGazeDragging) {
          isGazeDragging = false;
          try { gazeViewport.releasePointerCapture(e.pointerId); } catch (_) {}
        }
      };
      gazeViewport.addEventListener('pointerup', endGaze);
      gazeViewport.addEventListener('pointercancel', endGaze);
    }

    const btnCenterGaze = document.getElementById('btn-center-gaze');
    if (btnCenterGaze) {
      btnCenterGaze.addEventListener('click', () => {
        if (!gazeViewport) return;
        const rect = gazeViewport.getBoundingClientRect();
        handleGazeInteraction(rect.left + rect.width / 2, rect.top + rect.height / 2, 2000);
      });
    }

    /* Automatic Browser Time Synchronization (Supports AP mode & offline usage) */
    function syncDeviceTime() {
      const epoch = Math.floor(Date.now() / 1000);
      const tzOffsetSec = -new Date().getTimezoneOffset() * 60;
      fetch('/sync_time', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'epoch=' + epoch + '&tz=' + tzOffsetSec
      }).catch(function() {});
    }
    syncDeviceTime();

    /* Segmented Tab Switching */
    const tabBtns = document.querySelectorAll('.tab-btn');
    const tabPanes = document.querySelectorAll('.tab-pane');

    tabBtns.forEach(btn => {
      btn.addEventListener('click', function() {
        const targetId = this.dataset.target;
        tabBtns.forEach(b => b.classList.remove('active'));
        tabPanes.forEach(p => p.classList.remove('active'));
        this.classList.add('active');
        const targetPane = document.getElementById(targetId);
        if (targetPane) targetPane.classList.add('active');
        if (targetId === 'tab-control') {
          setTimeout(resizeGazeCanvas, 40);
        }
      });
    });

    setTimeout(resizeGazeCanvas, 80);

    /* Telemetry State & Periodic Fetch */
    let telemetryTimer = null;

    async function updateTelemetry() {
      if (document.visibilityState === 'hidden') {
        telemetryTimer = setTimeout(updateTelemetry, 1000);
        return;
      }

      try {
        const res = await fetch('/telemetry');
        if (res.ok) {
          const data = await res.json();

          const elFps = document.getElementById('tel-fps');
          const elConf = document.getElementById('tel-conf');
          const elHuman = document.getElementById('tel-human');
          const elProx = document.getElementById('tel-prox');

          if (elFps) elFps.innerText = (data.fps_ai !== undefined) ? Number(data.fps_ai).toFixed(1) : '0.0';
          if (elConf) elConf.innerText = (data.conf !== undefined) ? Number(data.conf).toFixed(2) : '0.00';
          if (elHuman) elHuman.innerText = (data.human_likelihood !== undefined) ? Number(data.human_likelihood).toFixed(2) : '0.00';
          if (elProx) elProx.innerText = (data.prox !== undefined) ? (Number(data.prox) * 100).toFixed(0) + '%' : '0%';

          // Live Brightness & Auto-Luminance Sync
          if (data.brightness !== undefined && isAutoBrightEnabled && !isBrightUpdating) {
            const b = parseInt(data.brightness, 10);
            if (brightSlider && brightSlider.value != b) brightSlider.value = b;
            const pct = Math.round((b / 255) * 100);
            if (brightBadge) brightBadge.innerText = `${b} (${pct}%)`;
          }
          if (data.auto_brightness !== undefined && btnAutoBright) {
            const autoEn = (data.auto_brightness === true || data.auto_brightness === 'true');
            if (autoEn !== isAutoBrightEnabled) {
              isAutoBrightEnabled = autoEn;
              if (autoEn) {
                btnAutoBright.classList.add('active');
                btnAutoBright.innerText = 'Auto-Brightness: ON';
              } else {
                btnAutoBright.classList.remove('active');
                btnAutoBright.innerText = 'Auto-Brightness: OFF';
              }
            }
          }

          // Update Cognitive & Neural State
          const elThought = document.getElementById('tel-thought');
          const elBonding = document.getElementById('tel-bonding');
          const elEnergy = document.getElementById('tel-energy');
          const elCuriosity = document.getElementById('tel-curiosity');

          if (elThought && data.thought) elThought.innerText = `"${data.thought}"`;
          if (elBonding && data.bonding !== undefined) elBonding.innerText = (Number(data.bonding) * 100).toFixed(0) + '%';
          if (elEnergy && data.circadian && data.circadian.energy !== undefined) elEnergy.innerText = (Number(data.circadian.energy) * 100).toFixed(0) + '%';
          if (elCuriosity && data.curiosity !== undefined) elCuriosity.innerText = (Number(data.curiosity) * 100).toFixed(0) + '%';

          // Update Robot Gaze Position & Trajectory on Radar
          if (data.cx !== undefined && data.cy !== undefined) {
            const fw = data.fw || 640;
            const fh = data.fh || 480;
            const normRx = (data.cx / fw) * 2.0 - 1.0;
            const normRy = (data.cy / fh) * 2.0 - 1.0;
            robotGazeState.x = Math.max(-1.0, Math.min(1.0, normRx));
            robotGazeState.y = Math.max(-1.0, Math.min(1.0, normRy));
            robotGazeState.detected = !!data.detected;

            const now = Date.now();
            gazeTrail.push({ x: robotGazeState.x, y: robotGazeState.y, time: now });
            gazeTrail = gazeTrail.filter(p => now - p.time < 1200);
            if (gazeTrail.length > 20) gazeTrail.shift();

            if (!userGazeTarget.active || Date.now() >= userGazeTarget.activeUntil) {
              if (gazeCoordsBadge) {
                const sx = robotGazeState.x >= 0 ? '+' : '';
                const sy = robotGazeState.y >= 0 ? '+' : '';
                gazeCoordsBadge.innerText = `X: ${sx}${robotGazeState.x.toFixed(2)} | Y: ${sy}${robotGazeState.y.toFixed(2)}`;
              }
            }
          }
          renderGazeRadar();

          /* Update active expression indicator */
          const isManual = (data.is_manual === true || data.is_manual === 'true');
          const currentExpr = (data.expr !== undefined) ? parseInt(data.expr, 10) : -1;
          const btnAuto = document.getElementById('btn-expr-auto');

          if (!isManual) {
            if (btnAuto) btnAuto.classList.add('active');
            document.querySelectorAll('.btn-expr').forEach(btn => btn.classList.remove('active'));
          } else {
            if (btnAuto) btnAuto.classList.remove('active');
            document.querySelectorAll('.btn-expr').forEach(btn => {
              if (parseInt(btn.dataset.expr, 10) === currentExpr) {
                btn.classList.add('active');
              } else {
                btn.classList.remove('active');
              }
            });
          }
        }
      } catch (e) {
        console.error('Telemetry fetch error:', e);
      } finally {
        telemetryTimer = setTimeout(updateTelemetry, 150);
      }
    }

    document.addEventListener('visibilitychange', () => {
      if (document.visibilityState === 'visible') {
        if (telemetryTimer) clearTimeout(telemetryTimer);
        updateTelemetry();
      }
    });

    updateTelemetry();

    /* Expression Control Logic */
    document.querySelectorAll('.btn-expr').forEach(btn => {
      btn.addEventListener('click', function() {
        const exprId = this.dataset.expr;
        document.querySelectorAll('.btn-expr').forEach(b => b.classList.remove('active'));
        const btnAuto = document.getElementById('btn-expr-auto');
        if (btnAuto) btnAuto.classList.remove('active');
        this.classList.add('active');

        fetch('/set_expression', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ expr: parseInt(exprId, 10) })
        }).catch(() => {});
      });
    });

    const btnExprAuto = document.getElementById('btn-expr-auto');
    if (btnExprAuto) {
      btnExprAuto.addEventListener('click', function() {
        document.querySelectorAll('.btn-expr').forEach(b => b.classList.remove('active'));
        this.classList.add('active');

        fetch('/set_expression', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ expr: 'auto' })
        }).catch(() => {});
      });
    }

    /* Wi-Fi Configuration Logic */
    fetch('/get_wifi')
      .then(res => res.json())
      .then(data => {
        if (data.sta_ssid) document.getElementById('sta_ssid').value = data.sta_ssid;
        if (data.ap_ssid) document.getElementById('ap_ssid').value = data.ap_ssid;
        if (data.ble_name) document.getElementById('ble_name').value = data.ble_name;
        document.getElementById('sta_pass').placeholder = 'Saved (leave blank to keep unchanged)';
        document.getElementById('ap_pass').placeholder = 'Saved (leave blank to keep unchanged)';

        const modeBadge = document.getElementById('mode-badge');
        const netBadgeInfo = document.getElementById('net-badge-info');
        const btnSwitch = document.getElementById('btn-switch-mode');
        
        if (data.is_ap) {
          modeBadge.innerText = 'AP Mode';
          if (netBadgeInfo) netBadgeInfo.innerText = 'Access Point Active';
          btnSwitch.innerText = 'Switch to STA Mode';
          btnSwitch.dataset.targetMode = 'STA';
        } else {
          modeBadge.innerText = 'STA Online';
          if (netBadgeInfo) netBadgeInfo.innerText = (data.sta_ssid || 'Local Wi-Fi');
          btnSwitch.innerText = 'Switch to AP Mode';
          btnSwitch.dataset.targetMode = 'AP';
        }
        btnSwitch.style.display = 'block';
      })
      .catch(() => {});

    document.getElementById('btn-switch-mode').addEventListener('click', function() {
      const targetMode = this.dataset.targetMode;
      const btn = this;
      const status = document.getElementById('status');
      
      btn.disabled = true;
      document.getElementById('btn-save').disabled = true;
      status.style.display = 'block';

      const unifiedUrl = 'http://192.168.18.16';

      if (targetMode === 'AP') {
        btn.innerText = 'Switching to AP Mode...';
        status.innerHTML = 'ESP32 is rebooting into AP Mode.<br>Connect Wi-Fi to <b>LoRe</b> then visit: <a href="' + unifiedUrl + '" style="color:#111827;font-weight:700;">' + unifiedUrl + '</a><br><small>Redirecting in 4 seconds...</small>';

        fetch('/switch_mode', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ mode: targetMode })
        }).finally(() => {
          setTimeout(() => { window.location.href = unifiedUrl; }, 4000);
        });
      } else {
        const staSsid = document.getElementById('sta_ssid').value || 'WiFi Router';
        btn.innerText = 'Switching to STA Mode...';
        status.innerHTML = 'ESP32 is rebooting and connecting to <b>' + staSsid + '</b>.<br>Visit URL: <a href="' + unifiedUrl + '" style="color:#111827;font-weight:700;">' + unifiedUrl + '</a><br><small>Redirecting in 6 seconds...</small>';

        fetch('/switch_mode', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ mode: targetMode })
        }).finally(() => {
          setTimeout(() => { window.location.href = unifiedUrl; }, 6000);
        });
      }
    });

    document.getElementById('wifi-form').addEventListener('submit', function(e) {
      e.preventDefault();
      const btn = document.getElementById('btn-save');
      const status = document.getElementById('status');

      const apPass = document.getElementById('ap_pass').value;
      if (apPass.length > 0 && apPass.length < 8) {
        status.style.display = 'block';
        status.style.color = '#ef4444';
        status.innerText = 'Error: AP password must be empty (Open AP) or at least 8 characters.';
        return;
      }

      status.style.color = 'var(--text-main)';
      btn.disabled = true;
      document.getElementById('btn-switch-mode').disabled = true;
      btn.innerHTML = '<span>Saving...</span>';
      status.style.display = 'block';

      const staSsid = document.getElementById('sta_ssid').value || 'WiFi Router';
      const unifiedUrl = 'http://192.168.18.16';
      status.innerHTML = 'Configuration saved. ESP32 is rebooting and connecting to <b>' + staSsid + '</b>.<br>Visit URL: <a href="' + unifiedUrl + '" style="color:#111827;font-weight:700;">' + unifiedUrl + '</a><br><small>Redirecting in 6 seconds...</small>';

      const body = {
        sta_ssid: document.getElementById('sta_ssid').value,
        sta_pass: document.getElementById('sta_pass').value,
        ap_ssid: document.getElementById('ap_ssid').value,
        ap_pass: document.getElementById('ap_pass').value,
        ble_name: document.getElementById('ble_name').value
      };

      fetch('/save_wifi', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(body)
      }).finally(() => {
        setTimeout(() => { window.location.href = unifiedUrl; }, 6000);
      });
    });

    /* Legal Disclosures Toggle */
    const toggleLegal = () => {
      const modal = document.getElementById('legal-content');
      modal.style.display = (modal.style.display === 'block') ? 'none' : 'block';
    };
    document.getElementById('toggle-privacy').addEventListener('click', toggleLegal);
    document.getElementById('toggle-terms').addEventListener('click', toggleLegal);

    /* Auto-Brightness Control */
    const btnAutoBright = document.getElementById('btn-auto-bright-toggle');
    let isAutoBrightEnabled = false;
    if (btnAutoBright) {
      btnAutoBright.addEventListener('click', function() {
        isAutoBrightEnabled = !isAutoBrightEnabled;
        if (isAutoBrightEnabled) {
          this.classList.add('active');
          this.innerText = 'Auto-Brightness: ON';
        } else {
          this.classList.remove('active');
          this.innerText = 'Auto-Brightness: OFF';
        }
        fetch('/set_auto_brightness', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ enabled: isAutoBrightEnabled })
        })
        .then(res => res.json())
        .then(data => {
          if (data && data.brightness !== undefined) {
            const b = parseInt(data.brightness, 10);
            if (brightSlider) brightSlider.value = b;
            const pct = Math.round((b / 255) * 100);
            if (brightBadge) brightBadge.innerText = `${b} (${pct}%)`;
          }
        })
        .catch(() => {});
      });
    }

    /* Ambient Screen Glance Triggers */
    const btnTrigClock = document.getElementById('btn-trigger-clock');
    if (btnTrigClock) {
      btnTrigClock.addEventListener('click', function() {
        fetch('/trigger_clock', { method: 'POST' }).catch(() => {});
      });
    }

    const btnTrigWeather = document.getElementById('btn-trigger-weather');
    if (btnTrigWeather) {
      btnTrigWeather.addEventListener('click', function() {
        fetch('/trigger_weather', { method: 'POST' }).catch(() => {});
      });
    }

    /* Live OLED Brightness Slider (Throttled, Queued & Non-Blocking) */
    const brightSlider = document.getElementById('oled-brightness-slider');
    const brightBadge = document.getElementById('bright-badge');
    let isBrightUpdating = false;
    let queuedBrightness = null;
    let lastBrightSendMs = 0;

    function sendBrightnessRequest(val, save) {
      const now = Date.now();
      if (!save) {
        if (isBrightUpdating || (now - lastBrightSendMs < 80)) {
          queuedBrightness = val;
          return;
        }
      }
      isBrightUpdating = true;
      lastBrightSendMs = now;
      queuedBrightness = null;

      fetch('/set_brightness', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ brightness: val, save: !!save })
      }).catch(() => {}).finally(() => {
        isBrightUpdating = false;
        if (queuedBrightness !== null) {
          const nextVal = queuedBrightness;
          queuedBrightness = null;
          sendBrightnessRequest(nextVal, false);
        }
      });
    }

    if (brightSlider) {
      brightSlider.addEventListener('input', function() {
        if (isAutoBrightEnabled && btnAutoBright) {
          isAutoBrightEnabled = false;
          btnAutoBright.classList.remove('active');
          btnAutoBright.innerText = 'Auto-Brightness: OFF';
        }
        const val = parseInt(this.value, 10);
        const pct = Math.round((val / 255) * 100);
        if (brightBadge) brightBadge.innerText = `${val} (${pct}%)`;
        sendBrightnessRequest(val, false);
      });
      brightSlider.addEventListener('change', function() {
        if (isAutoBrightEnabled && btnAutoBright) {
          isAutoBrightEnabled = false;
          btnAutoBright.classList.remove('active');
          btnAutoBright.innerText = 'Auto-Brightness: OFF';
        }
        const val = parseInt(this.value, 10);
        const pct = Math.round((val / 255) * 100);
        if (brightBadge) brightBadge.innerText = `${val} (${pct}%)`;
        sendBrightnessRequest(val, true);
      });
    }

    const btnResetBright = document.getElementById('btn-reset-brightness');
    if (btnResetBright) {
      btnResetBright.addEventListener('click', function() {
        if (isAutoBrightEnabled && btnAutoBright) {
          isAutoBrightEnabled = false;
          btnAutoBright.classList.remove('active');
          btnAutoBright.innerText = 'Auto-Brightness: OFF';
        }
        if (brightSlider) brightSlider.value = 128;
        if (brightBadge) brightBadge.innerText = '128 (50%)';
        sendBrightnessRequest(128, true);
      });
    }

    /* Weather Presets Map */
    const weatherPresets = {
      jakarta: { city: 'Jakarta', lat: -6.2088, lon: 106.8456, tz: 25200 },
      bandung: { city: 'Bandung', lat: -6.9175, lon: 107.6191, tz: 25200 },
      surabaya: { city: 'Surabaya', lat: -7.2575, lon: 112.7521, tz: 25200 },
      semarang: { city: 'Semarang', lat: -6.9667, lon: 110.4167, tz: 25200 },
      yogyakarta: { city: 'Yogyakarta', lat: -7.7956, lon: 110.3695, tz: 25200 },
      solo: { city: 'Surakarta', lat: -7.5666, lon: 110.8167, tz: 25200 },
      malang: { city: 'Malang', lat: -7.9797, lon: 112.6304, tz: 25200 },
      bogor: { city: 'Bogor', lat: -6.5950, lon: 106.8166, tz: 25200 },
      depok: { city: 'Depok', lat: -6.4025, lon: 106.7942, tz: 25200 },
      tangerang: { city: 'Tangerang', lat: -6.1783, lon: 106.6319, tz: 25200 },
      bekasi: { city: 'Bekasi', lat: -6.2383, lon: 106.9756, tz: 25200 },
      medan: { city: 'Medan', lat: 3.5952, lon: 98.6722, tz: 25200 },
      palembang: { city: 'Palembang', lat: -2.9761, lon: 104.7754, tz: 25200 },
      padang: { city: 'Padang', lat: -0.9471, lon: 100.4172, tz: 25200 },
      pekanbaru: { city: 'Pekanbaru', lat: 0.5071, lon: 101.4478, tz: 25200 },
      lampung: { city: 'B. Lampung', lat: -5.4500, lon: 105.2667, tz: 25200 },
      batam: { city: 'Batam', lat: 1.1301, lon: 104.0529, tz: 25200 },
      aceh: { city: 'Banda Aceh', lat: 5.5483, lon: 95.3238, tz: 25200 },
      bali: { city: 'Denpasar', lat: -8.6705, lon: 115.2126, tz: 28800 },
      mataram: { city: 'Mataram', lat: -8.5833, lon: 116.1167, tz: 28800 },
      kupang: { city: 'Kupang', lat: -10.1772, lon: 123.6070, tz: 28800 },
      ikn: { city: 'IKN Nusantara', lat: -0.9744, lon: 116.7027, tz: 28800 },
      balikpapan: { city: 'Balikpapan', lat: -1.2379, lon: 116.8289, tz: 28800 },
      samarinda: { city: 'Samarinda', lat: -0.5022, lon: 117.1536, tz: 28800 },
      banjarmasin: { city: 'Banjarmasin', lat: -3.3194, lon: 114.5908, tz: 28800 },
      pontianak: { city: 'Pontianak', lat: -0.0263, lon: 109.3425, tz: 25200 },
      makassar: { city: 'Makassar', lat: -5.1477, lon: 119.4327, tz: 28800 },
      manado: { city: 'Manado', lat: 1.4748, lon: 124.8428, tz: 28800 },
      jayapura: { city: 'Jayapura', lat: -2.5337, lon: 140.7181, tz: 32400 },
      ambon: { city: 'Ambon', lat: -3.6954, lon: 128.1814, tz: 32400 },
      singapore: { city: 'Singapore', lat: 1.3521, lon: 103.8198, tz: 28800 },
      kualalumpur: { city: 'Kuala Lumpur', lat: 3.1390, lon: 101.6869, tz: 28800 },
      tokyo: { city: 'Tokyo', lat: 35.6762, lon: 139.6503, tz: 32400 },
      seoul: { city: 'Seoul', lat: 37.5665, lon: 126.9780, tz: 32400 },
      london: { city: 'London', lat: 51.5074, lon: -0.1278, tz: 0 },
      newyork: { city: 'New York', lat: 40.7128, lon: -74.0060, tz: -18000 },
      paris: { city: 'Paris', lat: 48.8566, lon: 2.3522, tz: 3600 },
      dubai: { city: 'Dubai', lat: 25.2048, lon: 55.2708, tz: 14400 },
      sydney: { city: 'Sydney', lat: -33.8688, lon: 151.2093, tz: 36000 }
    };

    const presetSel = document.getElementById('weather-preset-select');
    const tzSel = document.getElementById('weather-tz-select');

    function syncWeatherPresetSelection(city, lat, lon) {
      if (!presetSel) return;
      let matchedKey = 'custom';
      const parsedLat = parseFloat(lat);
      const parsedLon = parseFloat(lon);
      const cleanCity = (city || '').trim().toLowerCase();

      for (const [key, p] of Object.entries(weatherPresets)) {
        const cityMatch = cleanCity && (p.city.toLowerCase() === cleanCity || key === cleanCity);
        const coordMatch = !isNaN(parsedLat) && !isNaN(parsedLon) &&
                           Math.abs(p.lat - parsedLat) < 0.015 &&
                           Math.abs(p.lon - parsedLon) < 0.015;
        if (cityMatch || coordMatch) {
          matchedKey = key;
          break;
        }
      }
      presetSel.value = matchedKey;
    }

    if (presetSel) {
      presetSel.addEventListener('change', function() {
        const key = this.value;
        if (weatherPresets[key]) {
          document.getElementById('weather-city-input').value = weatherPresets[key].city;
          document.getElementById('weather-lat-input').value = weatherPresets[key].lat.toFixed ? weatherPresets[key].lat.toFixed(4) : weatherPresets[key].lat;
          document.getElementById('weather-lon-input').value = weatherPresets[key].lon.toFixed ? weatherPresets[key].lon.toFixed(4) : weatherPresets[key].lon;
          if (tzSel && weatherPresets[key].tz !== undefined) {
            tzSel.value = weatherPresets[key].tz;
          }
        }
      });
    }

    ['weather-city-input', 'weather-lat-input', 'weather-lon-input'].forEach(id => {
      const el = document.getElementById(id);
      if (el) {
        el.addEventListener('input', () => {
          syncWeatherPresetSelection(
            document.getElementById('weather-city-input').value,
            document.getElementById('weather-lat-input').value,
            document.getElementById('weather-lon-input').value
          );
        });
      }
    });

    /* Live Open-Meteo Geocoding Autocomplete Search */
    const searchInput = document.getElementById('weather-search-input');
    const searchResults = document.getElementById('weather-search-results');
    let searchDebounceTimer = null;

    if (searchInput && searchResults) {
      searchInput.addEventListener('input', function() {
        const q = this.value.trim();
        clearTimeout(searchDebounceTimer);
        if (q.length < 2) {
          searchResults.style.display = 'none';
          searchResults.innerHTML = '';
          return;
        }
        searchDebounceTimer = setTimeout(() => {
          fetch(`https://geocoding-api.open-meteo.com/v1/search?name=${encodeURIComponent(q)}&count=8&language=en&format=json`)
            .then(res => res.json())
            .then(data => {
              if (data && data.results && data.results.length > 0) {
                searchResults.innerHTML = '';
                data.results.forEach(item => {
                  const div = document.createElement('div');
                  div.style.padding = '8px 12px';
                  div.style.cursor = 'pointer';
                  div.style.borderBottom = '1px solid var(--border-card)';
                  div.style.fontSize = '12px';
                  div.style.transition = 'background 0.15s';
                  div.innerHTML = `<b>${item.name}</b> <span style="color:var(--text-muted);">(${[item.admin1, item.country].filter(Boolean).join(', ')})</span><br><small style="color:var(--text-muted);font-size:10.5px;">Lat: ${item.latitude.toFixed(4)}, Lon: ${item.longitude.toFixed(4)}</small>`;
                  div.addEventListener('mouseenter', () => { div.style.background = 'var(--bg-surface)'; });
                  div.addEventListener('mouseleave', () => { div.style.background = 'transparent'; });
                  div.addEventListener('click', () => {
                    document.getElementById('weather-city-input').value = item.name.substring(0, 16);
                    document.getElementById('weather-lat-input').value = item.latitude.toFixed(4);
                    document.getElementById('weather-lon-input').value = item.longitude.toFixed(4);
                    searchResults.style.display = 'none';
                    searchInput.value = `${item.name}, ${item.country || ''}`;
                    syncWeatherPresetSelection(item.name, item.latitude, item.longitude);
                  });
                  searchResults.appendChild(div);
                });
                searchResults.style.display = 'block';
              } else {
                searchResults.innerHTML = '<div style="padding:8px 12px;font-size:11.5px;color:var(--text-muted);">City not found</div>';
                searchResults.style.display = 'block';
              }
            }).catch(() => {
              searchResults.style.display = 'none';
            });
        }, 300);
      });

      document.addEventListener('click', function(e) {
        if (!searchInput.contains(e.target) && !searchResults.contains(e.target)) {
          searchResults.style.display = 'none';
        }
      });
    }

    /* GPS One-Click Location Finder */
    const btnGps = document.getElementById('btn-gps-location');
    const gpsPinSvg = '<svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 10c0 7-9 13-9 13s-9-6-9-13a9 9 0 0 1 18 0z"></path><circle cx="12" cy="10" r="3"></circle></svg>';

    if (btnGps) {
      btnGps.addEventListener('click', function() {
        if (!navigator.geolocation) {
          alert('Geolocation is not supported by your browser.');
          return;
        }
        btnGps.disabled = true;
        btnGps.innerHTML = gpsPinSvg + ' <span>Locating...</span>';
        navigator.geolocation.getCurrentPosition(
          pos => {
            btnGps.disabled = false;
            btnGps.innerHTML = gpsPinSvg + ' <span>My GPS</span>';
            const lat = pos.coords.latitude.toFixed(4);
            const lon = pos.coords.longitude.toFixed(4);
            document.getElementById('weather-lat-input').value = lat;
            document.getElementById('weather-lon-input').value = lon;
            document.getElementById('weather-city-input').value = 'GPS Location';
            if (presetSel) presetSel.value = 'custom';
            fetch(`https://api.bigdatacloud.net/data/reverse-geocode-client?latitude=${lat}&longitude=${lon}&localityLanguage=en`)
              .then(r => r.json())
              .then(geo => {
                if (geo.city || geo.locality) {
                  const cName = (geo.city || geo.locality || 'GPS Location').substring(0, 16);
                  document.getElementById('weather-city-input').value = cName;
                  if (searchInput) searchInput.value = `${cName}, ${geo.countryName || ''}`;
                  syncWeatherPresetSelection(cName, lat, lon);
                }
              }).catch(() => {});
          },
          err => {
            btnGps.disabled = false;
            btnGps.innerHTML = gpsPinSvg + ' <span>My GPS</span>';
            alert('Failed to retrieve GPS: ' + err.message);
          },
          { timeout: 8000, enableHighAccuracy: true }
        );
      });
    }

    function loadWeatherAndSystemSettings() {
      fetch('/weather_info')
        .then(res => res.json())
        .then(data => {
          if (data.city) document.getElementById('weather-city-input').value = data.city;
          if (data.lat !== undefined) document.getElementById('weather-lat-input').value = data.lat.toFixed ? data.lat.toFixed(4) : data.lat;
          if (data.lon !== undefined) document.getElementById('weather-lon-input').value = data.lon.toFixed ? data.lon.toFixed(4) : data.lon;
          if (data.enabled !== undefined) document.getElementById('weather-enable-check').checked = data.enabled;
          if (data.tz_offset_sec !== undefined && tzSel) tzSel.value = data.tz_offset_sec;

          syncWeatherPresetSelection(data.city, data.lat, data.lon);

          const infoBox = document.getElementById('weather-live-info');
          const badge = document.getElementById('weather-badge-status');
          if (data.valid) {
            if (badge) badge.innerText = `${data.temp}°C, ${data.condition}`;
            if (infoBox) {
              infoBox.style.display = 'block';
              infoBox.innerHTML = `<b>${data.city}:</b> ${data.temp}°C, ${data.condition} &bull; Humidity: ${data.humidity}% (Synced: ${data.last_sync_s}s ago)`;
            }
          }
        }).catch(() => {});

      fetch('/system_info')
        .then(res => res.json())
        .then(data => {
          if (data.brightness !== undefined && brightSlider) {
            brightSlider.value = data.brightness;
            const pct = Math.round((data.brightness / 255) * 100);
            if (brightBadge) brightBadge.innerText = `${data.brightness} (${pct}%)`;
          }
          if (data.auto_brightness !== undefined && btnAutoBright) {
            isAutoBrightEnabled = !!data.auto_brightness;
            if (isAutoBrightEnabled) {
              btnAutoBright.classList.add('active');
              btnAutoBright.innerText = 'Auto-Brightness: ON';
            } else {
              btnAutoBright.classList.remove('active');
              btnAutoBright.innerText = 'Auto-Brightness: OFF';
            }
          }
        }).catch(() => {});
    }

    loadWeatherAndSystemSettings();

    const btnSaveWeather = document.getElementById('btn-save-weather');
    if (btnSaveWeather) {
      btnSaveWeather.addEventListener('click', function() {
        const btn = this;
        btn.disabled = true;
        btn.innerText = 'Saving...';

        const body = {
          city: document.getElementById('weather-city-input').value,
          lat: parseFloat(document.getElementById('weather-lat-input').value),
          lon: parseFloat(document.getElementById('weather-lon-input').value),
          enabled: document.getElementById('weather-enable-check').checked,
          tz_offset_sec: parseInt(document.getElementById('weather-tz-select').value, 10)
        };

        fetch('/set_weather', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(body)
        }).finally(() => {
          setTimeout(() => {
            btn.disabled = false;
            btn.innerText = 'Save & Fetch Weather →';
            loadWeatherAndSystemSettings();
          }, 1500);
        });
      });
    }

    const btnPreviewClock = document.getElementById('btn-preview-clock');
    if (btnPreviewClock) {
      btnPreviewClock.addEventListener('click', function() {
        const orig = btnPreviewClock.innerText;
        btnPreviewClock.innerText = 'Showing...';
        btnPreviewClock.disabled = true;
        fetch('/trigger_clock', { method: 'POST' })
          .catch(() => {})
          .finally(() => {
            setTimeout(() => {
              btnPreviewClock.innerText = orig;
              btnPreviewClock.disabled = false;
            }, 5000);
          });
      });
    }

    const btnPreviewWeather = document.getElementById('btn-preview-weather');
    if (btnPreviewWeather) {
      btnPreviewWeather.addEventListener('click', function() {
        const orig = btnPreviewWeather.innerText;
        btnPreviewWeather.innerText = 'Showing...';
        btnPreviewWeather.disabled = true;
        fetch('/trigger_weather', { method: 'POST' })
          .catch(() => {})
          .finally(() => {
            setTimeout(() => {
              btnPreviewWeather.innerText = orig;
              btnPreviewWeather.disabled = false;
            }, 5000);
          });
      });
    }

    /* Ntfy & Notification Management */
    function loadNtfySettings() {
      fetch('/api/ntfy')
        .then(res => res.json())
        .then(data => {
          if (data && data.topic) {
            const topicInput = document.getElementById('ntfy-topic-input');
            if (topicInput) topicInput.value = data.topic;
            const codeNtfy = document.getElementById('code-ntfy-url');
            if (codeNtfy) codeNtfy.innerText = `https://ntfy.sh/${data.topic}`;
            const badge = document.getElementById('ntfy-status-badge');
            if (badge) {
              badge.innerText = data.connected ? 'ntfy: connected' : 'ntfy: standby';
              badge.style.color = data.connected ? 'var(--accent-dark)' : 'var(--text-muted)';
            }
          }
        }).catch(() => {});
      
      const codeLocal = document.getElementById('code-local-url');
      if (codeLocal) codeLocal.innerText = `http://${window.location.host}/api/notify`;
    }

    loadNtfySettings();

    const btnSaveNtfy = document.getElementById('btn-save-ntfy');
    if (btnSaveNtfy) {
      btnSaveNtfy.addEventListener('click', function() {
        const topic = (document.getElementById('ntfy-topic-input').value || '').trim();
        if (!topic) return;
        btnSaveNtfy.disabled = true;
        btnSaveNtfy.innerText = 'Saving...';
        fetch('/api/ntfy', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ topic: topic })
        }).finally(() => {
          setTimeout(() => {
            btnSaveNtfy.disabled = false;
            btnSaveNtfy.innerText = 'Save Topic';
            loadNtfySettings();
          }, 800);
        });
      });
    }

    const btnTestNotif = document.getElementById('btn-test-notif');
    if (btnTestNotif) {
      btnTestNotif.addEventListener('click', function() {
        const orig = btnTestNotif.innerText;
        btnTestNotif.innerText = 'Sending...';
        btnTestNotif.disabled = true;
        fetch('/api/notify', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            app: 'WhatsApp',
            sender: 'Budi (Test)',
            message: 'Halo! Notifikasi LoRe berhasil terhubung!'
          })
        }).finally(() => {
          setTimeout(() => {
            btnTestNotif.innerText = orig;
            btnTestNotif.disabled = false;
          }, 4000);
        });
      });
    }

</script>
</body>
</html>
)rawliteral";

#endif /* WEB_UI_H */
