---
name: Hexloom Studio
description: A warm creative workshop where agents help shape playable worlds.
colors:
  evening-ink: "#191814"
  workshop-surface: "#211F1A"
  raised-workbench: "#2A2821"
  pencil-border: "#3B382F"
  parchment-text: "#F0E9DC"
  quiet-text: "#AAA193"
  clay-apricot: "#E7A978"
  mist-blue: "#86AEB4"
  honey-amber: "#D9BE79"
  fault-coral: "#D9897E"
  calm-sage: "#9EB29A"
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
    backgroundColor: "{colors.clay-apricot}"
    textColor: "{colors.evening-ink}"
    typography: "{typography.label}"
    rounded: "{rounded.control}"
    padding: "10px 14px"
  button-ghost:
    backgroundColor: "{colors.workshop-surface}"
    textColor: "{colors.parchment-text}"
    typography: "{typography.label}"
    rounded: "{rounded.control}"
    padding: "10px 14px"
  panel:
    backgroundColor: "{colors.workshop-surface}"
    textColor: "{colors.parchment-text}"
    rounded: "{rounded.panel}"
    padding: "18px"
  status-live:
    backgroundColor: "{colors.raised-workbench}"
    textColor: "{colors.clay-apricot}"
    typography: "{typography.label}"
    rounded: "{rounded.pill}"
    padding: "5px 9px"
---

# Design System: Hexloom Studio

## 1. Overview

**Creative North Star: "The Evening Maker's Desk"**

Hexloom is a low-glare creative workspace for a solo creator working on a
laptop in a warm room at night, shaping a world while a friend watches it take
form. It should feel like a carefully kept maker's desk: calm, tactile,
personal, and visibly alive only when real work is happening.

The interface combines familiar product structure with the character of a
digital atelier. Warm tinted neutrals, soft geometry, plain-language labels,
honest event streams, and asymmetric workspace regions create confidence
without aggression. It explicitly rejects generic neon cyberpunk dashboards,
poisonous AI-green accents, fake terminal walls, Matrix rain, crypto-trading
interfaces, glassmorphism, identical SaaS card grids, and decorative AI
animations.

**Key Characteristics:**

- Dense but immediately scannable.
- Warm and low-glare, never black, fluorescent, or clinical.
- Asymmetric workspace composition with stable navigation.
- Monospace reserved for paths, commands, and compact telemetry.
- Visual energy produced by live, inspectable work.

## 2. Colors

The palette is a warm evening workshop with restrained clay, mist, honey, and
sage signals.

### Primary

- **Clay Apricot:** Reserved for the current creative action, keyboard focus,
  and the one primary control in a region.

### Secondary

- **Mist Blue:** Identifies information, linked artifacts, and agent output
  that is available for inspection.

### Tertiary

- **Honey Amber:** Marks waiting, review, warnings, and decisions that
  require the operator.
- **Fault Coral:** Marks failed work and destructive actions without flooding
  the whole surface.

### Neutral

- **Evening Ink:** The application canvas and deepest working layer.
- **Workshop Surface:** Persistent rails, panels, and quiet controls.
- **Raised Workbench:** Selected rows, active tool areas, and nested working
  surfaces.
- **Pencil Border:** Fine structural separators and inactive outlines.
- **Parchment Text:** Primary readable content.
- **Quiet Text:** Metadata and secondary explanations.

### Named Rules

**The Warm Hand Rule.** Clay Apricot occupies no more than ten percent of a
screen; it marks creation and invitation, never generic system activity.

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
- **Primary:** Clay Apricot fill, Evening Ink text, and compact 10px by 14px
  padding. Only one primary action appears in a region.
- **Hover / Focus:** Borders brighten over 180ms; focus adds a visible
  two-pixel outer signal.
- **Secondary / Ghost:** Tonal surface and a one-pixel Pencil Border outline.

### Chips

- **Style:** Quiet tonal fill, explicit label, and optional semantic dot or
  symbol.
- **State:** Selection changes both fill and text; state never depends on color
  alone.

### Cards / Containers

- **Corner Style:** Workbench panels use a restrained 10px radius.
- **Background:** Workshop Surface for persistent regions and Raised Workbench for
  selected or nested content.
- **Shadow Strategy:** Persistent containers use no shadow.
- **Border:** One-pixel Pencil Border separators.
- **Internal Padding:** 12px for dense lists and 18px for primary panels.

### Inputs / Fields

- **Style:** Evening Ink inset field, one-pixel border, and 6px corners.
- **Focus:** Clay Apricot border plus a visible external focus cue.
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

### Artifact Preview

The selected game artifact owns the largest region of the workspace. Models
render in a real native 3D viewport with neutral studio lighting, enough
environment to judge silhouette, and no decorative HUD. The generation plan
sits beside the preview as supporting context, never on top of the work.

## 6. Do's and Don'ts

### Do:

- **Do** reserve Clay Apricot for creative actions, focus, and the primary
  action.
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
- **Don't** use poisonous AI-green accents as the default sign of intelligence
  or activity.
- **Don't** build fake walls of terminal noise or Matrix rain.
- **Don't** imitate crypto-trading interfaces.
- **Don't** use glassmorphism.
- **Don't** arrange the workspace as identical SaaS card grids.
- **Don't** add decorative AI animations that imply activity without
  explaining it.
- **Don't** make Hexloom resemble a movie prop, a reskinned chat application,
  an observability dashboard, or an admin panel with green text.
- **Don't** use colored side stripes, gradient text, or nested cards as visual
  shortcuts.
