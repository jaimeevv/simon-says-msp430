# Technical report — LaTeX sources

Sources for [`../simon-says-msp430.pdf`](../simon-says-msp430.pdf).

| File | Purpose |
| --- | --- |
| `report.tex` | Main document |
| `main.c` | Game source, included verbatim in the "Source Code" section (`\lstinputlisting`) |
| `img-000.jpg` | Title-page photo |

`main.c` here is the as-submitted snapshot of the game code. The buildable Code
Composer Studio copy lives in [`../../Proyecto/main.c`](../../Proyecto/main.c); the two
are functionally identical and differ only in comments and formatting.

## Compile

```
tectonic report.tex
```

or with a full TeX Live / MiKTeX install:

```
pdflatex report.tex && pdflatex report.tex
```

Overleaf: upload the three files and set `report.tex` as the main document.
