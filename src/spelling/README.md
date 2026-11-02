Built-in source spelling uses the SCOWL English dictionary distributed in Debian
wamerican 2020.12.07-4, including contractions and possessives, plus entries from
`@cspell/dict-software-terms` 5.4.8 (npm tarball SHA-1
`65d2701b170551e37e1b669fbae9ff41737bc28a`). Original notices are included here
and packaged in both applications. Sources:
https://github.com/en-wl/wordlist and
https://github.com/streetsidesoftware/cspell-dicts/tree/main/dictionaries/software-terms.

Run `python3 tools/generate_studio_spelling.py` to regenerate the embedded Buoy
module and Xippets dictionary. Runtime checking is offline: no host dictionary,
Node process, or separate dictionary installation is required. Both editors use the same generated English, programmer and Buoy
keyword vocabulary with local hash lookups and bounded one-edit suggestions.
Neither application interprets Hunspell affix rules.

Comments, strings and identifiers are checked. Camel/Pascal case, acronym
boundaries, underscores, hyphens and digits separate words. Words of at most two
letters, language tokens and technical vocabulary are accepted. URLs, email,
paths and non-English words are excluded; valid escape sequences are skipped
only inside strings.
This is English spelling, with no automatic symbol renaming or autocorrection.
A correction changes the selected word component through the normal undo path;
use Rename Symbol if you want to update its references as well.

Studio checks visible lines on a short timer and retains a bounded content/state
cache with individual eviction and memoized line fingerprints. Spelling has its
own underlines and never enters compiler diagnostics.
View > Editor > Live Spell Check persists the toggle; right-click a flagged word
for suggestions or Ignore in This Editor. Ignore lasts for that editor instance.
The Messages pane opts out. Xippets uses delayed CodeMirror lint decorations
with visible-line checking for large buffers and a persistent Settings toggle.
Hover a flagged word for suggestions and Ignore. Its host API exposes
`setSpellCheck(boolean)` and `setDocument({spellCheck:false,...})`.

The spelling cache owns arrays through `StudioSpellingCacheEntry` to avoid the
Buoy 0.72 optional-class-array read leak tracked as BUI-818. Search error paths
keep throwing-call receivers in named locals (BUI-817). Native editor/input
registries clear owning properties instead of assigning Nil to class-array
elements (BUI-506). The search and spelling runners build debug tests and reject
ARC leak reports on supported debug hosts; Linux GUI runs also use the GTK guard.
