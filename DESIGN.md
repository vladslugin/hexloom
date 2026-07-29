---
name: Hexloom Studio
description: A calm operator's workshop where agents weave playable worlds.
colors:
  void-graphite: "#0B0F0D"
  loom-surface: "#111713"
  raised-workbench: "#17201B"
  thread-border: "#29362F"
  woven-text: "#D9E4DE"
  quiet-text: "#83968C"
  live-phosphor: "#67E6A3"
  signal-cyan: "#73B8D4"
  attention-amber: "#E5B566"
  fault-coral: "#E27D76"
typography:
  title:
    fontFamily: "Inter, SF Pro Text, Segoe UI, system-ui, sans-serif"
    fontSize: "20px"
    fontWeight: 650
    lineHeight: 1.25
    letterSpacing: "-0.01em"
  body:
    fontFamily: "Inter, SF Pro Text, Segoe UI, system-ui, sans-serif"
    fontSize: "14px"
    fontWeight: 400
    lineHeight: 1.5
    letterSpacing: "normal"
  label:
    fontFamily: "SFMono-Regular, Cascadia Mono, Consolas, monospace"
    fontSize: "11px"
    fontWeight: 600
    lineHeight: 1.3
    letterSpacing: "0.08em"
rounded:
  control: "6px"
  panel: "10px"
  pill: "999px"
spacing:
  xs: "4px"
  sm: "8px"
  md: "12px"
  lg: "18px"
  xl: "24px"
components:
  button-primary:
    backgroundColor: "{colors.live-phosphor}"
    textColor: "{colors.void-graphite}"
    typography: "{typography.label}"
    rounded: "{rounded.control}"
    padding: "10px 14px"
  button-ghost:
    backgroundColor: "{colors.loom-surface}"
    textColor: "{colors.woven-text}"
    typography: "{typography.label}"
    rounded: "{rounded.control}"
    padding: "10px 14px"
  panel:
    backgroundColor: "{colors.loom-surface}"
    textColor: "{colors.woven-text}"
    rounded: "{rounded.panel}"
    padding: "18px"
  status-live:
    backgroundColor: "{colors.raised-workbench}"
    textColor: "{colors.live-phosphor}"
    typography: "{typography.label}"
    rounded: "{rounded.pill}"
    padding: "5px 9px"
---

# Design System: Hexloom Studio

## 1. Overview

**Creative North Star: "The Living Workbench"**

Hexloom is a low-glare instrument for a solo creator working on a laptop at
night, moving between agent streams while a collaborator watches the system
work. It should feel like a beautifully maintained electronics bench: dense
with meaningful signals, calm at rest, and visibly alive only when real work
is happening.

The interface combines familiar product structure with the character of a
technical atelier. Fine separators, compact labels, honest event streams, and
asymmetric workspace regions create the hacker appeal. It explicitly rejects
generic neon cyberpunk dashboards, fake terminal walls, Matrix rain,
crypto-trading interfaces, glassmorphism, identical SaaS card grids, and
decorative AI animations.

**Key Characteristics:**

- Dense but immediately scannable.
- Dark and low-glare, never black and fluorescent.
- Asymmetric workspace composition with stable navigation.
- Monospace reserved for machines, state, paths, and commands.
- Visual energy produced by live, inspectable work.

## 2. Colors

The palette is a warm graphite workshop with a restrained set of instrument
signals.

### Primary

- **Live Phosphor:** Reserved for the current action, successful execution,
  keyboard focus, and the one primary control in a region.

### Secondary

- **Signal Cyan:** Identifies information, linked artifacts, and agent output
  that is available for inspection.

### Tertiary

- **Attention Amber:** Marks waiting, review, warnings, and decisions that
  require the operator.
- **Fault Coral:** Marks failed work and destructive actions without flooding
  the whole surface.

### Neutral

- **Void Graphite:** The application canvas and deepest terminal layer.
- **Loom Surface:** Persistent rails, panels, and quiet controls.
- **Raised Workbench:** Selected rows, active tool areas, and nested working
  surfaces.
- **Thread Border:** Fine structural separators and inactive outlines.
- **Woven Text:** Primary readable content.
- **Quiet Text:** Metadata and secondary explanations.

### Named Rules

**The Live Wire Rule.** Live Phosphor occupies no more than ten percent of a
screen; its rarity makes active work unmistakable.

**The Signal Pair Rule.** Status color is always paired with an icon, label, or
explicit state word.

## 3. Typography

**Display Font:** Inter with native system sans fallbacks  
**Body Font:** Inter with native system sans fallbacks  
**Label/Mono Font:** SF Mono with Cascadia Mono and Consolas fallbacks

**Character:** The sans voice is neutral, compact, and dependable. Monospace
creates machine character without turning every sentence into terminal output.

### Hierarchy

- **Title** (650, 20px, 1.25): Workspace and artifact titles.
- **Body** (400, 14px, 1.5): Explanations, agent messages, and inspector
  content; prose is limited to 70 characters where practical.
- **Label** (600, 11px, 0.08em): Uppercase section labels, statuses, paths, and
  compact controls.

### Named Rules

**The Two Voices Rule.** Human guidance uses sans; machine identity, state,
paths, commands, and telemetry use mono.

## 4. Elevation

Hexloom is flat by default. Depth is communicated through tonal surfaces,
one-pixel borders, overlap, and spatial hierarchy rather than ambient card
shadows. A restrained shadow may appear only on floating command surfaces or
dragged artifacts.

### Shadow Vocabulary

- **Floating Command:** A broad, low-opacity shadow separates temporary command
  surfaces from the workspace without suggesting glass.

### Named Rules

**The Bench Rule.** Persistent surfaces never float. If every panel casts a
shadow, the hierarchy is wrong.

## 5. Components

### Buttons

- **Shape:** Compact, gently machined corners (6px radius).
- **Primary:** Live Phosphor fill, Void Graphite text, and compact 10px by 14px
  padding. Only one primary action appears in a region.
- **Hover / Focus:** Borders brighten over 180ms; focus adds a visible
  two-pixel outer signal.
- **Secondary / Ghost:** Tonal surface and a one-pixel Thread Border outline.

### Chips

- **Style:** Quiet tonal fill, explicit label, and optional semantic dot or
  symbol.
- **State:** Selection changes both fill and text; state never depends on color
  alone.

### Cards / Containers

- **Corner Style:** Workbench panels use a restrained 10px radius.
- **Background:** Loom Surface for persistent regions and Raised Workbench for
  selected or nested content.
- **Shadow Strategy:** Persistent containers use no shadow.
- **Border:** One-pixel Thread Border separators.
- **Internal Padding:** 12px for dense lists and 18px for primary panels.

### Inputs / Fields

- **Style:** Void Graphite inset field, one-pixel border, and 6px corners.
- **Focus:** Live Phosphor border plus a visible external focus cue.
- **Error / Disabled:** Fault Coral plus an error label; disabled controls lose
  contrast but retain readable labels.

### Navigation

The left project rail and top context bar remain stable. Active destinations
use a tonal selection, a leading glyph, and clear text. Compact layouts
collapse labels before hiding navigation.

### Agent Stream

Every event displays agent identity, semantic event type, relative time, and a
plain-language result. Tool calls can expand to show command, scope, and raw
output. Running events use restrained activity motion; completed events become
quiet history instead of continuing to glow.

## 6. Do's and Don'ts

### Do:

- **Do** reserve Live Phosphor for active work, focus, success, and primary
  actions.
- **Do** connect every agent event to its originating intent and resulting
  artifact.
- **Do** use asymmetric regions to establish priority while keeping navigation
  positions stable.
- **Do** provide reduced-motion behavior and non-color state labels.
- **Do** let genuine builds, tests, diffs, and generated previews create the
  spectacle.

### Don't:

- **Don't** create generic neon cyberpunk dashboards or use fluorescent color
  as decoration.
- **Don't** build fake walls of terminal noise or Matrix rain.
- **Don't** imitate crypto-trading interfaces.
- **Don't** use glassmorphism.
- **Don't** arrange the workspace as identical SaaS card grids.
- **Don't** add decorative AI animations that imply activity without
  explaining it.
- **Don't** make Hexloom resemble a movie prop, a reskinned chat application,
  or an admin dashboard with green text.
- **Don't** use colored side stripes, gradient text, or nested cards as visual
  shortcuts.
