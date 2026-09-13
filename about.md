# PracticeMe

**PracticeMe** — your personal practice assistant for Geometry Dash.

PracticeMe observes your Practice Mode gameplay and discovers which parts of a level you actually struggle with — then tells you exactly what to practice.

> **Don't repeatedly practice the entire level. Practice the parts you're actually bad at.**

## Features
- **Level Menu Button** — open PracticeMe from the level info screen
- **Smart Section Analysis** — deterministic, lightweight algorithm detecting frequent deaths, fail rate, consistency
- **Recommended Practice** — HIGH / MEDIUM / LOW / GOOD priorities with death/attempt/success stats
- **Adaptive Learning** — as you improve, priorities lower automatically
- **Session History** — lightweight history of best progress per session
- **Data Confidence** — NO DATA / LOW / MEDIUM / HIGH so you know when to trust recommendations
- **Reliable Level Identity** — official & custom levels tracked separately, never mixing data
- **Persistent Lightweight Storage** — compact JSON, survives restarts, handles corruption gracefully

## Usage
1. Play a level in **Practice Mode** for a few attempts
2. Go to the level info screen and press the **PracticeMe** button
3. See your recommended practice ranges and weak sections

If no data exists, PracticeMe shows `NO DATA FOUND` and explains to play first.

Built by **KOBI** with Geode.
