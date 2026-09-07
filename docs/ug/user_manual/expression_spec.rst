.. _expression_spec:

Trigger and complete expressions
////////////////////////////////

This is a technical, reference-level specification of the *expression* language used by the ``trigger`` and
``complete`` attributes of a suite definition. This specification is intended to be authoritative and parser-agnostic.

..  implementation::

    In practice, the specification was derived directly from the source code (i.e. the expression lexer/parser and the
    abstract syntax tree it produces) and describes the grammar the ecFlow server actually accepts, but is independent
    of the actual parser implementation in use.

.. _expr-introduction:

Introduction
============

A ``trigger`` or ``complete`` attribute attaches a boolean *expression* to a node (``suite``, ``family``, ``task`` or
``alias``). The expression is a condition over the current state of the definition tree, and it controls scheduling:

- **trigger** — a *gate*.

    A node with a trigger is held in the ``queued`` state and cannot start until its trigger expression evaluates to
    *true*.

- **complete** — a *shortcut*.

    When a complete expression evaluates to *true*, the associated node is treated as ``complete`` without being run
    (i.e. its children are not scheduled).

Both attributes share exactly the same expression language; the only difference is how the resulting truth value is
used by the scheduler.

.. note::

    If a node has both a trigger and a complete expression, the complete expression is evaluated and acted upon
    *before* the trigger expression is evaluated.

.. code-block:: shell

    # trigger on the state of another node
    task t1
        trigger t0 == complete

    # complete this task early when a sibling has already finished
    task t2
        trigger  ./prepare == complete
        complete ./retrieve == complete

Scope
-----

Expressions may reference, by :ref:`node path <expr-operands>`, any node in the definition and any of its attributes
(events, meters, variables, limits, repeats, flags). References may be absolute (``/suite/family/task``) or relative to
the node carrying the expression (``./sibling``, ``../uncle``). Referencing a node outside the current definition
requires a matching ``extern``.

An expression yields an integer; a **non-zero** result is *true* and a **zero** result is *false*. Comparisons and
boolean operators yield ``1`` or ``0``, but a bare numeric operand is also a valid expression (for example ``trigger
1`` is always true, and ``trigger /suite/limit:remaining`` is true while the value is non-zero).

Abstract syntax tree
--------------------

.. implementation::

    An expression string is parsed **once** into an *abstract syntax tree* (AST) — a tree of ``Ast`` nodes rooted at an
    ``AstTop`` — and the AST is cached on the owning node. Thereafter every scheduling cycle simply *evaluates* the
    cached tree; the text is never re-parsed.

    Parsing is deferred: loading a checkpoint (``.check``) stores only the expression *string*, and the AST is built
    lazily on first use (or eagerly by ``Defs::check()`` when a text ``.def`` is loaded). Because large operational
    suites contain hundreds of thousands of expressions, this one-off parse is a measurable load-time cost.

Understanding the tree shape helps predict how an expression is evaluated. The *flat* rendering below shows the
fully-bracketed AST for each example (operators are shown in their canonical symbolic form):

.. list-table:: Introductory examples
   :header-rows: 1
   :widths: 45 55

   * - *Flat* Expression
     - Parsed as (fully bracketed)
   * - ``t0 == complete``
     - ``(t0 == complete)`` — trigger on a node *state*
   * - ``/data/get:ready == set``
     - ``(/data/get:ready == set)`` — trigger on an *event*
   * - ``t:step >= 240``
     - ``(t:step >= 240)`` — trigger on a *meter* (or any numeric attribute)
   * - ``a == complete and b == complete``
     - ``((a == complete) and (b == complete))`` — several conditions
   * - ``./model == complete or ./model == aborted``
     - ``((./model == complete) or (./model == aborted))``
   * - ``:YMD == 20240101``
     - ``(:YMD == 20240101)`` — complete on a *date* (a ``repeat`` date variable)
   * - ``(obs:YMD + 1) <= /o/main:YMD``
     - ``((obs:YMD + 1) <= /o/main:YMD)`` — arithmetic on date variables

.. _expr-concepts:

Concepts
========

An expression is built from **operands** combined by **operators**, optionally grouped with parentheses. This section
introduces the pieces — see the :ref:`expr-grammar` section for their exact syntax.

.. important::

    The expression language is **case-sensitive**.

    Every keyword — node and event states (``complete``, ``set``, …), boolean and word-form comparison operators
    (``and``, ``or``, ``not``, ``eq``, ``ne``, …) and function names — is recognised only in the exact casing shown.

    A few operators do have an additional all-uppercase spelling (``AND``, ``OR``); these are separate, exact
    spellings, not a relaxation of case-sensitivity.

    Mixed-case forms of the operators, such as ``Or`` or ``NOT``, are rejected. For example, ``and`` is a valid operator
    but ``AnD`` is not, and ``complete`` is a valid state whereas ``Complete`` is not.


.. _expr-operands:

Operands
--------

.. list-table::
   :header-rows: 1
   :widths: 24 30 46

   * - Operand
     - Example
     - Meaning / value
   * - Node state
     - ``/suite/task``
     - Only valid on the left of a state comparison (``== complete``). Resolves to the node's status.
   * - Node attribute
     - ``/path/to/node:name``
     - The integer value of the named attribute of the node at ``/path/to/node``: an *event* (``set`` = 1,
       ``clear`` = 0), a *meter*, a user or generated *variable*, a *limit*, a *repeat*, or a *queue*.
   * - Parent attribute
     - ``:name``
     - The value of the attribute ``name``, resolved exactly like a *node attribute* but located **by name up the
       ancestor chain**: the nearest of this node and its ancestors (up to the suite) that has an event, meter,
       variable, repeat, generated variable, limit or queue called ``name``. Commonly used for inherited or generated
       variables such as ``:YMD``.
   * - Integer
     - ``240``
     - A non-negative integer literal. Integers of the form ``YYYYMMDD`` (e.g. ``20240101``) are commonly used to
       represent dates.
   * - Date/time instant
     - ``20240101T060000``
     - A calendar instant, ``YYYYMMDD`` followed by ``T`` and ``hhmmss``. Enables calendar-aware comparison and
       arithmetic against date/time variables.
   * - Flag
     - ``path<flag>late``
     - A boolean: 1 when the given flag (``late``, ``zombie`` or ``archived``) is set on the node at ``path``.
   * - Calendar function
     - ``cal::date_to_julian( path:YMD )``
     - ``date_to_julian`` converts a ``YYYYMMDD`` value to a Julian day number; ``julian_to_date`` is the inverse. The
       single argument must be a node attribute (``path:name``) or an integer.

Operators
---------

.. list-table::
   :header-rows: 1
   :widths: 22 34 44

   * - Class
     - Operators
     - Notes
   * - Boolean AND
     - ``and``   ``AND``   ``&&``
     - Higher precedence operator than ``OR``.
   * - Boolean OR
     - ``or``   ``OR``   ``||``
     - Lower precedence operator than ``AND``.
   * - Boolean NOT
     - ``not``   ``!``   ``~``
     - Unary, applies to the following comparison or parenthesised group.
       While the word form ``not`` must be followed by whitespace (``not (a == complete)``),
       the symbolic forms ``!`` and ``~`` do not require a separating space.
   * - Equality
     - ``==``   ``eq``   ``!=``   ``ne``
     - The **only** operators allowed against a *node state* or *event state*; also usable numerically.
   * - Relational
     - ``<``  ``lt``   ``<=``  ``le``   ``>``  ``gt``   ``>=``  ``ge``
     - **Numeric only** — both sides must be arithmetic expressions. ``t < complete`` is *not* valid.
   * - Arithmetic
     - ``+``   ``-``   ``*``   ``/``   ``%``
     - Arithmetic operators have **no** internal precedence — see :ref:`expr-precedence`.

The symbolic and word forms are interchangeable (``==`` and ``eq`` are the same operator).

Division or modulo by zero is rejected when the definition is checked **only if the divisor can be evaluated to zero at
that point** — that is, a literal ``0`` (``x / 0``) or an operand (such as a variable, meter, limit or repeat) whose
*current* value is zero. Because the check evaluates the operands' present values, a divisor that is non-zero at check
time passes, even if it may later become zero; conversely a divisor that happens to be zero at check time is rejected
even if it would normally be non-zero.

.. implementation::

   A zero divisor encountered at *runtime* (during trigger/complete evaluation) does **not** abort evaluation or raise
   an error to the user: the division or modulo is logged (``Divide by zero in trigger/complete expression``) and the
   offending sub-expression yields ``0``. Evaluation of the surrounding expression then continues with that ``0`` — so,
   for example, ``10 / v == 0`` becomes *true* when ``v`` is ``0``.

Combining and grouping
-----------------------

Within an expression, conditions are combined with the boolean operators and, optionally, grouped with parentheses:

.. code-block:: shell

    trigger (a == complete or b == complete) and c == complete

A single node may also build up one logical expression from **several** ``trigger`` (or ``complete``) lines, using the
``-a`` (AND) and ``-o`` (OR) modifiers. The first line must be unmodified; subsequent lines are combined with the
running expression:

.. code-block:: shell

    task t
        trigger    a == complete
        trigger -a b == complete     # AND a == complete and b == complete
        trigger -o c == complete     # OR  ... or c == complete

.. note::

   The ``-a`` / ``-o`` modifiers are part of the *definition-file* syntax for accumulating parts, not part of the
   expression grammar itself. The definition-file parser records each line as a separate part (tagged *first*, *and* or
   *or*); it neither concatenates them into one string nor merges them. Each part is then parsed **independently** by
   the expression parser into its own tree, and a separate aggregation step joins those trees under fresh ``and`` /
   ``or`` nodes to form the node's single combined expression. The expression parser itself only ever sees one part at
   a time and is unaware of the other lines.

.. _expr-grammar:

Grammar
=======

This section is the reference grammar, and describes the language accepted. It is written in Extended Backus–Naur form
(EBNF) over two layers — a *lexical* layer that turns the source string into tokens, and a *syntactic* layer that
combines tokens into an expression.

Lexical analysis
----------------

The lexer first turns the raw expression string into a stream of *tokens* (names, integers, quoted literals, keywords
and operator symbols). Two rules govern how that split is made — in particular, when a run of characters is treated as a
keyword rather than as part of a name.

Tokenisation is **greedy** (*longest match*): the lexer consumes the longest run that forms a single :token:`name` or
:token:`integer` before any keyword is considered. A keyword is therefore recognised only when it stands as a complete
token, delimited by whitespace, a bracket, an operator symbol or the end of the string. Consequently, tokens such as
``completeand`` and ``515and`` are each a single :token:`name` — **not** ``complete``/``515`` followed by ``and`` — so
a keyword adjacent to an operand must be separated by whitespace (``complete and``, ``515 and``).

This separation requirement applies **only to the word-form keywords** (the states, and the word-form operators
``and``, ``or``, ``not``, ``eq``, ``ne``, ``lt`` …), because those are :token:`name`-shaped. The **symbolic** operators
(``==``, ``!=``, ``<``, ``<=``, ``+``, ``&&`` …) are not made of name characters and always self-delimit, so they never
require surrounding whitespace: ``5<=6`` and ``a==complete`` are valid, whereas the word forms must be spaced
(``5 le 6``, ``a eq complete``).

Lexical elements
----------------

Whitespace (spaces and tabs) separates tokens and is otherwise insignificant, **except** that a :token:`node_path`,
:token:`attribute`, :token:`flag_ref` and :token:`datetime` are single lexemes and may not contain interior
whitespace.

.. productionlist:: expr-lexical
   name      : (letter | digit | "_") { letter | digit | "_" | "." }
   integer   : digit { digit }
   datetime  : digit digit digit digit digit digit digit digit "T"
             : digit digit digit digit digit digit
   flag      : "<flag>"

A :token:`name` therefore matches the pattern ``\w(\w|\.)*`` (a leading letter, digit or underscore, then letters,
digits, underscores or dots) — the same rule as node and attribute names elsewhere in the definition file. A run of
digits followed immediately by letters (e.g. ``4dvar``) is a :token:`name`, not an :token:`integer`.

Keywords
--------

The following words are recognised as operators or literals in the positions shown by the grammar. They are recognised
only as complete tokens (never as a prefix of a longer :token:`name`); there is otherwise no reserved-word list (a node
may legitimately be named ``complete``, and ``complete == complete`` is a valid comparison of that node against the
state).

.. list-table::
   :header-rows: 1
   :widths: 28 72

   * - Category
     - Words
   * - Node states
     - ``complete``  ``aborted``  ``queued``  ``submitted``  ``active``  ``unknown``
   * - Event states
     - ``set``  ``clear``
   * - Flags
     - ``late``  ``zombie``  ``archived``
   * - Boolean
     - ``and``  ``AND``  ``or``  ``OR``  ``not``
   * - Comparison (word forms)
     - ``eq``  ``ne``  ``lt``  ``le``  ``gt``  ``ge``
   * - Functions
     - ``cal::date_to_julian``  ``cal::julian_to_date``

Syntactic grammar
-----------------

.. productionlist:: expr
   expression       : subexpression
   subexpression    : head [ (and | or) subexpression ]
   head             : [ not ] ( "(" subexpression ")" | and_expr )
   and_expr         : comparison { and [ not ] comparison }
   comparison       : node_path eq_op node_state
                    : | attribute eq_op event_state
                    : | calc_expr [ cmp_op [ not ] calc_expr ]
   calc_expr        : calc_factor { arith_op calc_factor }
   calc_factor      : datetime
                    : | integer
                    : | "(" calc_expr ")"
                    : | parent_attribute
                    : | cal_function
                    : | flag_ref
                    : | attribute
   attribute        : node_path ":" name
   parent_attribute : ":" name
   flag_ref         : ( node_path | "/" ) flag flag_name
   cal_function     : ("cal::date_to_julian" | "cal::julian_to_date") "(" ( attribute | integer ) ")"
   node_path        : absolute_path | dot_path | dotdot_path
   absolute_path    : [ "/" ] name { "/"+ name }
   dot_path         : "." "/" name { "/" name }
   dotdot_path      : ".." { "/"+ ".." } { "/"+ name }
   node_state       : "complete" | "aborted" | "queued" | "submitted" | "active" | "unknown"
   event_state      : "set" | "clear"
   flag_name        : "late" | "zombie" | "archived"
   and              : "and" | "AND" | "&&"
   or               : "or" | "OR" | "||"
   not              : "not" | "!" | "~"
   eq_op            : "==" | "eq" | "!=" | "ne"
   cmp_op           : eq_op | "<" | "lt" | "<=" | "le" | ">" | "gt" | ">=" | "ge"
   arith_op         : "+" | "-" | "*" | "/" | "%"

Notes on the productions:

- A **node-state** comparison (``node_path eq_op node_state``) accepts only the equality operators, and only a bare
  node path on the left. Relational operators (``<``, ``>=`` …) and node states are never mixed: ``t < complete`` is
  rejected.
- A bare :token:`node_path` on its own (e.g. ``/a/b``) is **not** a valid operand — it must appear in a state
  comparison. A bare :token:`attribute` (``a/b:step``), :token:`parent_attribute` (``:YMD``), :token:`integer`,
  :token:`datetime` or :token:`flag_ref` *is* a valid, standalone expression.
- There is **no unary sign**: ``-5`` and ``+x`` are not accepted; a leading ``+``/``-`` is only ever a binary
  arithmetic operator.

  .. implementation::

     The parser accepts a leading operator syntactically (a prefix :token:`calc_factor`), but the resulting binary
     node has no second operand, so the expression is rejected when the definition is checked — ``-5`` is therefore
     never valid in practice.
- The single argument of a ``cal::`` function must be a :token:`attribute` (``path:name``) or an
  :token:`integer` — a :token:`parent_attribute` (``:name``) is not accepted there.

.. _expr-precedence:

Precedence and associativity
----------------------------

.. hint::

    Mixed boolean precedence/associativity is subtle — add parentheses to make evaluation order explicit.

The following list presents the order of precedence, from highest to lowest binding:

#. Parentheses and function calls.
#. Unary ``not`` / ``!`` / ``~``.
#. Arithmetic ``+ - * / %`` — **all equal precedence, left-associative**. There is *no* multiplicative-over-additive
   precedence, so ``1 + 2 * 3`` parses as ``((1 + 2) * 3)``, evaluating to ``9``. Use parentheses to force the intended
   grouping, e.g. ``1 + (2 * 3)``.
#. Comparison (equality and relational) — a comparison joins two arithmetic expressions and does not chain.
#. Boolean ``and``.
#. Boolean ``or``.

Boolean associativity is deliberately **asymmetric**:

- A run of comparisons joined only by ``and`` is **left-associative**: ``a==complete and b==complete and c==complete``
  parses as ``(((a==complete) and (b==complete)) and (c==complete))``.
- Once an ``or`` — or a parenthesised boolean group — is involved, the surrounding heads are chained
  **right-associatively** by the mix of ``and``/``or``. Thus ``1==1 and 2==2 or 3==3`` parses as
  ``(((1==1) and (2==2)) or (3==3))``.

Worked examples
---------------

.. list-table::
   :header-rows: 1
   :widths: 50 50

   * - Expression
     - Parsed as (fully bracketed)
   * - ``a == complete``
     - ``(a == complete)``
   * - ``t:step + 20 ge t:step1 - 20``
     - ``((t:step + 20) >= (t:step1 - 20))``
   * - ``cal::date_to_julian(/o/main:YMD) == cal::date_to_julian(20240101)``
     - ``(date_to_julian(/o/main:YMD) == date_to_julian(20240101))``
   * - ``not ./model == complete``
     - ``not ((./model == complete))``
   * - ``(a==complete or b==complete) and c==complete``
     - ``(((a==complete) or (b==complete)) and (c==complete))``
   * - ``:YMD + 1 == 20240102``
     - ``((:YMD + 1) == 20240102)``

.. note::

   The ``cal::`` row above shows a *simplified* rendering. A ``cal::`` function node prints its **evaluated**
   argument and result rather than the source operand, so the actual flat rendering of that example is
   ``(date_to_julian(arg:0) = 0 == date_to_julian(arg:20240101) = 2460311)`` (the ``arg:`` value and the computed
   Julian day depend on the current state). The bracketing of the surrounding expression is unchanged.
