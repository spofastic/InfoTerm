# Language System

InfoTerm is prepared for a multilingual user interface.

The language definitions are located in:

```text
include/Language.h
```

The firmware currently includes German and English text entries. New languages can be added by extending the language table and adding translated text constants.

## Design Goal

Visible UI text should not be hard-coded across the firmware. Instead, display labels, web interface text and menu entries should reference centralized language keys.

This makes future translation and maintenance easier.
