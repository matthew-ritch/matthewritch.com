---
layout: post
title: "Estimating disease prevalence by fusing pooled and individual testing"
date: 2026-07-24 10:00:00 -0400
categories: blog
tags: [bayesian statistics, probability, epidemiology]
excerpt: Turning our arXiv paper into an interactive walk-through. Combine pooled and individual disease tests into a single closed-form Bayesian posterior for population prevalence sans MCMC.
image: /images/fusion-og.png
---


* TOC
{:toc}

Binary disease tests are used to estimate disease prevalence in a population. When test kits are scarce, researchers and epidemiologists may choose to test samples pooled from multiple people. Further, with observational studies and meta-analyses, you may end up with results from both individual and pooled samples. To combine these results into a single estimate of the population prevalence, some prior work leapt to using compute-heavy and slow MCMC methods. We (Charles Copley, my colleague from Ancera, and I) found a closed-form posterior distribution for the population prevalence that can be computed in a fraction of a second. This post is an overview of that work. The full derivation + analysis is [on arXiv](https://arxiv.org/abs/2308.11041).


## Why pool samples at all?

Let's say we are trying to measure a population prevalence $$p$$. If you were to sample $$n$$ individuals, then you would see about $$np$$ positives. Now, what if $$p=.01$$? Well you would need to sample 100 individuals to see on average 1 positive result. We can boost that signal by pooling.

The probability of a pooled sample testing positive when $$q$$ individuals are pooled, $$\pi _q$$, is equivalent to the probability that at least one of the samples which are combined into the pooled sample is positive. We'll assume that each individual's positivity is i.i.d. Bernoulli.

$$\pi_q = 1 - \Pr(\text{all }q\text{ samples negative}) = 1 - (1-p)^q$$

Or, if you want to see it visually:

<figure class="pv-fig" id="pv-pool">
  <div class="pv-controls">
    <label>Prevalence <span class="pv-var">p</span>
      <input type="range" id="pool-p" min="0.01" max="0.99" step="0.01" value="0.10">
      <output id="pool-p-out">0.10</output>
    </label>
    <label>Pool size <span class="pv-var">q</span>
      <input type="range" id="pool-q" min="1" max="10" step="1" value="5">
      <output id="pool-q-out">5</output>
    </label>
    <button type="button" class="pv-btn" id="pool-resample">🎲 Resample pool</button>
  </div>
  <div class="pv-pool-readout">
    <div class="pv-chips" id="pool-chips" aria-hidden="true"></div>
    <div class="pv-pool-status">
      <div>Pooled test: <strong id="pool-result">—</strong></div>
      <div class="pv-muted">\(\pi_q = 1-(1-p)^q = \) <span id="pool-pi">0.41</span></div>
    </div>
  </div>
  <canvas class="pv-canvas" id="pool-canvas" height="260"></canvas>
  <div class="pv-legend">
    <span class="lg"><i class="sw sw-line" style="color:var(--rb-s2)"></i>Pr(pooled positive), \(\pi_q\)</span>
    <span class="lg"><i class="sw sw-dot" style="color:var(--rb-ink)"></i>your current setting</span>
    <span class="lg"><i class="sw sw-dash" style="color:var(--rb-muted)"></i>prevalence <em>p</em></span>
  </div>
  <figcaption>Probability a pooled test comes back positive as a function of the true prevalence <em>p</em>, for a pool of <em>q</em> samples.</figcaption>
</figure>

Let's quantify the difference in information a single individual test yields vs. a single pooled test.

Say $$X$$ is a Bernoulli random variable representing the result of an individual test. Its likelihood function over possible outcomes $$k$$ is 

$$f(k \mid p) = p^k (1-p)^{1-k}$$

Say $$Y$$ is a Bernoulli random variable representing the result of a pooled test. Its likelihood function over possible outcomes $$k$$ is 

$$g(k \mid p) = \bigg(1 - (1-p)^q\bigg)^k \bigg((1-p)^q\bigg)^{1-k}$$

[Fisher information](https://en.wikipedia.org/wiki/Fisher_information) $$I(p)$$ is calculated as the negative expected value of the second derivative of the log-likelihood. It measures how much reads of a RV tell us about an unknown parameter of its distribution.

For the individual test, 

$$I_X(p) =  \frac{1}{p} + \frac{1}{1-p} = \frac{1}{p(1-p)}$$

For the pooled test (thank you [Wolfram Alpha](https://www.wolframalpha.com/input?i2d=true&i=+Divide%5Bd%2Cdp%5D++Divide%5Bd%2Cdp%5D++++k*ln%5C%2840%291-Power%5B%5C%2840%291-p%5C%2841%29%2Cq%5D%5C%2841%29+%2B+%5C%2840%291-k%5C%2841%29ln%5C%2840%29Power%5B%5C%2840%291-p%5C%2841%29%2Cq%5D%5C%2841%29)),

$$\frac{\partial^2}{\partial p^2} \ln g(k \mid p) = -\frac{q\left(k(1-p)^q + kq(1-p)^q - k - 2(1-p)^q + (1-p)^{2q} + 1\right)}{(p-1)^2\left((1-p)^q - 1\right)^2}$$


so

$$I_Y(p) = \frac{q^2 (1-p)^{q-2}}{1-(1-p)^q}$$

Notice that our $$q=1$$ case matches $$I_X (p) $$. So, the information ratio between these is

$$\frac{I_Y(p)}{I_X(p)} = \frac{q^2 p (1-p)^{q-1}}{1-(1-p)^q}$$

<figure class="pv-fig" id="pv-info">
  <canvas class="pv-canvas" id="info-canvas" height="320"></canvas>
  <figcaption>How many individual tests one pooled test is worth, <em>I<sub>Y</sub>/I<sub>X</sub></em>, across p.</figcaption>
</figure>

The ratio approaches $$q$$ as $$p\rightarrow0$$, meaning that at low prevalences a pooled test of $$q$$ samples is up to $$q$$ times as informative as an individual sample, meaning that you can get the same info for about $$q$$ times as few samples.





## Fusing information from pooled and individual tests

Define $$P$$ as a random variable for population prevalence. We will use 
$$p$$ to denote outcome values of that random variable.

Define $$m$$ as the number of individual tests conducted.

Define $$Y_1,Y_2,...Y_m$$ as random variables for the binary results of these individual tests.

Define $$y$$ as the number of observed positive individual tests.

Define $$n$$ as the number of pooled tests conducted.

Define $$Z_1,Z_2,...Z_n$$ as random variables for the binary results of these pooled tests.

Define $$z$$ as the number of observed positive pooled tests.

As before, we'll use $$\pi_q$$ for the probability of a pooled sample testing positive when $$q$$ individuals are pooled.

In general, we assume a beta prior distribution for $$P$$
$$ P \sim Beta(\alpha, \beta) $$
so

$$ Pr(P=p) = \frac{ p^{\alpha - 1} (1-p)^{\beta - 1}}{B(\alpha, \beta)} $$

where $$B(\alpha, \beta)$$ is the beta function of $$\alpha$$ and $$\beta$$.

Bayes' Theorem can be stated for this problem as

$$ Pr(P=p|Y=y, Z=z) = \frac{Pr(Y=y, Z=z|P=p)Pr(P=p)}{Pr(Y=y, Z=z)} $$


$$Y \sim \text{Binomial}(m, p), \qquad Z \sim \text{Binomial}(n, \pi_q), \qquad P \sim \text{Beta}(\alpha,\beta)$$

At this point, the prior work plugged these equations into an MCMC solver. We will solve $$\Pr(P=p \mid Y=y, Z=z)$$ analytically instead.






## The closed form solution

For the full derivation, you should check out [our arxiv](https://arxiv.org/abs/2308.11041) post, but the gist is that you can use a binomial expansion of $$\pi_q^z$$ 

$$\label{pibin}  \pi _q ^{z} = [1 - (1-p)^{q}]^{z} = \sum _{i=0} ^{z} \binom{z}{i} (-1)^{i} (1-p)^{qi}$$

and linearity of integration to solve a closed form for the "evidence", $$Pr(Y=y, Z=z)$$. After a few pages of derivations, and substituting \\( \gamma = y+\alpha \\) and \\( \delta = m-y+\beta+qn-qz \\), we can derive the posterior distribution.

$$
\Pr(P=p \mid y, z) = \frac{\displaystyle\sum_{i=0}^{z}\binom{z}{i}(-1)^{i}\,p^{\gamma-1}(1-p)^{\delta+qi-1}}
{\displaystyle\sum_{i=0}^{z}\binom{z}{i}(-1)^{i}\,B(\gamma,\delta+qi)}
$$

This function is exact, integrable, and evaluates way faster than a MCMC chain. Further, if you need to include sensitivity or specificity, those generalize in the same way, even if you allow them to vary along another beta distribution.


## Weighted sum of betas

One cool thing about this equation is that we can rewrite it to reveal a bit of structure. The posterior is an affine combination of beta distributions!

Let $$f(p, \gamma, \delta + qi)$$ be the PDF of the beta distribution with parameters  $$\gamma$$, and $$\delta + qi$$, evaluated at $$p$$.

So we can also write the posterior probability distribution for $$P$$ in this form:


$$
\Pr(P=p \mid y, z) = \frac{\displaystyle\sum_{i=0}^{z}\binom{z}{i}(-1)^{i}\,B(\gamma,\delta+qi)\,f(p;\gamma,\delta+qi)}
{\displaystyle\sum_{i=0}^{z}\binom{z}{i}(-1)^{i}\,B(\gamma,\delta+qi)}
$$

The widget below shows a given posterior's weighted components and their sum. It also shows the closest-fitting single beta distribution.

<figure class="pv-fig" id="pv-betas">
  <div class="pv-controls">
    <label>Indiv. positives <span class="pv-var">y</span>
      <input type="range" id="b-y" min="0" max="10" step="1" value="1"><output id="b-y-out">0</output>
    </label>
    <label>Indiv. tests <span class="pv-var">m</span>
      <input type="range" id="b-m" min="1" max="10" step="1" value="5"><output id="b-m-out">1</output>
    </label>
    <label>Pool positives <span class="pv-var">z</span>
      <input type="range" id="b-z" min="0" max="10" step="1" value="4"><output id="b-z-out">1</output>
    </label>
    <label>Pooled tests <span class="pv-var">n</span>
      <input type="range" id="b-n" min="1" max="10" step="1" value="5"><output id="b-n-out">1</output>
    </label>
    <label>Pool size <span class="pv-var">q</span>
      <input type="range" id="b-q" min="2" max="5" step="1" value="3"><output id="b-q-out">3</output>
    </label>
  </div>
  <canvas class="pv-canvas" id="b-canvas" height="300"></canvas>
  <div class="pv-legend">
    <span class="lg"><i class="sw sw-line" style="color:var(--rb-ink)"></i>posterior (the signed sum)</span>
    <span class="lg"><i class="sw sw-line" style="color:var(--rb-s1)"></i>added Beta components (+)</span>
    <span class="lg"><i class="sw sw-line" style="color:var(--rb-s2)"></i>subtracted Beta components (−)</span>
    <span class="lg"><i class="sw sw-dash" style="color:var(--rb-muted)"></i>closest single Beta</span>
  </div>
</figure>


## The posterior distribution

Explore the posterior! Drag the sliders to set the experimental setting and see the resulting posterior. Hit resample to change the drawn data and resulting fit. You can see that pooled tests are more useful at low $$p$$ and that degraded sensitivity and specificity result in a wider posterior distribution.

<figure class="pv-fig" id="pv-play">
  <div class="pv-controls pv-controls-wide">
    <label>Individual tests <span class="pv-var">m</span>
      <input type="range" id="pg-m" min="0" max="60" step="1" value="20"><output id="pg-m-out">20</output>
    </label>
    <label>Pooled tests <span class="pv-var">n</span>
      <input type="range" id="pg-n" min="0" max="60" step="1" value="20"><output id="pg-n-out">20</output>
    </label>
    <label>Pool size <span class="pv-var">q</span>
      <input type="range" id="pg-q" min="1" max="8" step="1" value="5"><output id="pg-q-out">5</output>
    </label>
    <label>True prevalence <span class="pv-var">P</span>
      <input type="range" id="pg-P" min="0.01" max="0.99" step="0.01" value="0.05"><output id="pg-P-out">0.05</output>
    </label>
    <label>Sensitivity <span class="pv-var">s<sub>e</sub></span>
      <input type="range" id="pg-se" min="0.80" max="1.00" step="0.01" value="1.00"><output id="pg-se-out">1.00</output>
    </label>
    <label>Specificity <span class="pv-var">s<sub>p</sub></span>
      <input type="range" id="pg-sp" min="0.80" max="1.00" step="0.01" value="1.00"><output id="pg-sp-out">1.00</output>
    </label>
    <button type="button" class="pv-btn" id="pg-resample">🎲 Resample data</button>
  </div>
  <div class="pv-stats">
    <span>observed <span class="pv-var">y</span> = <strong id="pg-y">3</strong> / <span class="pv-var">m</span></span>
    <span>observed <span class="pv-var">z</span> = <strong id="pg-z">17</strong> / <span class="pv-var">n</span></span>
    <span>posterior mean = <strong id="pg-mean">—</strong></span>
    <span>95% CI = <strong id="pg-ci">—</strong></span>
  </div>
  <canvas class="pv-canvas" id="pg-canvas" height="300"></canvas>
  <div class="pv-legend">
    <span class="lg"><i class="sw sw-line" style="color:var(--rb-s1)"></i>posterior density</span>
    <span class="lg"><i class="sw sw-band" style="color:var(--rb-tent)"></i>95% credible interval</span>
    <span class="lg"><i class="sw sw-dot" style="color:var(--rb-ink)"></i>posterior mean</span>
    <span class="lg"><i class="sw sw-dash" style="color:var(--rb-s2)"></i>true prevalence</span>
  </div>
</figure>


## Budgeting fixed and pooled tests

So, practically, if you have a fixed test budget (who doesn't) and you expect low prevalence, you should consider pooling. Our paper shows extensive simulation of the tradeoffs here, so check it out if you're curious.

<!-- TODO(prose): the practical takeaway. Given a fixed test budget and a rough
     prior guess at prevalence, how should you split between pooled and individual
     tests, and what q? Pooled testing shines at low prevalence (tighter CIs per
     kit); at high prevalence pools saturate and wash out. Imperfect tests degrade
     everything, worst at the extremes of P. Tie back to the arXiv sims. -->





<!-- =====================  WIDGET STYLES  ===================== -->
<style>
.pv-fig {
  margin: 1.8em auto; max-width: 640px;
  font-family: system-ui, -apple-system, "Segoe UI", sans-serif;
  --rb-surface: #fcfcfb; --rb-ink: #0b0b0b; --rb-sec: #52514e; --rb-muted: #898781;
  --rb-axis: #c3c2b7; --rb-s1: #2a78d6; --rb-s2: #eb6834;
  --rb-line: #d3d2c8; --rb-head: rgba(137,135,129,0.12);
  --rb-tent: rgba(137,135,129,0.18);
}
/* Site is light-only (no prefers-color-scheme handling in its CSS), so the
   widgets stay light to match. Reintroduce a dark block here if the site ever
   grows a dark theme. */
.pv-canvas { width: 100%; display: block; background: var(--rb-surface);
  border: 1px solid var(--rb-line); border-radius: 6px; }
.pv-fig figcaption { font-size: 0.85em; color: var(--rb-sec); margin-top: 0.7em; line-height: 1.5; text-align: center; }
.pv-controls { display: grid; grid-template-columns: 1fr 1fr; gap: 0.6em 1.2em;
  margin-bottom: 1em; align-items: center; }
.pv-controls-wide { grid-template-columns: 1fr 1fr 1fr; }
@media (max-width: 480px) { .pv-controls, .pv-controls-wide { grid-template-columns: 1fr 1fr; } }
.pv-controls label { display: flex; flex-direction: column; gap: 0.2em;
  font-size: 0.72em; letter-spacing: 0.04em; text-transform: uppercase;
  color: var(--rb-muted); font-weight: 600; }
.pv-controls input[type="range"] { width: 100%; accent-color: var(--rb-s1); margin: 0; }
.pv-controls output { color: var(--rb-ink); font-weight: 700; font-size: 1.15em;
  font-variant-numeric: tabular-nums; text-transform: none; letter-spacing: 0; }
.pv-var { color: var(--rb-s1); font-style: italic; font-weight: 700; text-transform: none; }
.pv-btn { grid-column: 1 / -1; justify-self: start; cursor: pointer;
  background: var(--rb-head); color: var(--rb-ink); border: 1px solid var(--rb-line);
  border-radius: 6px; padding: 0.45em 0.9em; font: inherit; font-size: 0.85em; font-weight: 600; }
.pv-btn:hover { border-color: var(--rb-s1); }
.pv-stats { display: flex; flex-wrap: wrap; gap: 0.4em 1.4em; justify-content: center;
  margin-bottom: 0.9em; font-size: 0.85em; color: var(--rb-sec); font-variant-numeric: tabular-nums; }
.pv-stats strong { color: var(--rb-ink); }
.pv-muted { color: var(--rb-muted); }
.pv-pool-readout { display: flex; align-items: center; gap: 1.2em; margin-bottom: 1em; flex-wrap: wrap; }
.pv-chips { display: flex; gap: 6px; flex-wrap: wrap; }
.pv-chip { width: 26px; height: 26px; border-radius: 6px; border: 1px solid var(--rb-line);
  background: var(--rb-head); transition: background 0.15s; }
.pv-chip.pos { background: var(--rb-s2); border-color: var(--rb-s2); }
.pv-pool-status { font-size: 0.9em; color: var(--rb-sec); line-height: 1.6; }
.pv-pool-status strong { color: var(--rb-ink); }
.pv-legend { display: flex; flex-wrap: wrap; gap: 0.4em 1.3em; justify-content: center;
  margin-top: 0.8em; font-size: 0.82em; color: var(--rb-sec); }
.pv-legend .lg { display: inline-flex; align-items: center; gap: 0.5em; }
.pv-legend .sw { flex: none; display: inline-block; }
.pv-legend .sw-line { width: 20px; height: 0; border-top: 3px solid currentColor; }
.pv-legend .sw-dash { width: 20px; height: 0; border-top: 2px dashed currentColor; }
.pv-legend .sw-band { width: 20px; height: 12px; border-radius: 3px; background: currentColor; }
.pv-legend .sw-dot { width: 11px; height: 11px; border-radius: 50%;
  background: currentColor; box-shadow: 0 0 0 2px var(--rb-surface); }
</style>


<!-- =====================  WIDGET SCRIPT  ===================== -->
<script>
(function () {
  // ---------- numerics ----------
  var GC = [76.18009172947146, -86.50532032941677, 24.01409824083091,
            -1.231739572450155, 0.1208650973866179e-2, -0.5395239384953e-5];
  function logGamma(x) {
    var xx = x, y = x, tmp = x + 5.5;
    tmp -= (x + 0.5) * Math.log(tmp);
    var ser = 1.000000000190015;
    for (var j = 0; j < 6; j++) { y += 1; ser += GC[j] / y; }
    return -tmp + Math.log(2.5066282746310005 * ser / xx);
  }
  function logBeta(a, b) { return logGamma(a) + logGamma(b) - logGamma(a + b); }
  function betaPdf(p, a, b) {
    if (p < 0 || p > 1) return 0;
    // endpoint limits (a, b >= 1 here): density is finite, and equals 1/B when
    // the relevant shape parameter is exactly 1. Returning 0 here would kink the curve.
    if (p === 0) return a > 1 ? 0 : (a === 1 ? Math.exp(-logBeta(a, b)) : Infinity);
    if (p === 1) return b > 1 ? 0 : (b === 1 ? Math.exp(-logBeta(a, b)) : Infinity);
    return Math.exp((a - 1) * Math.log(p) + (b - 1) * Math.log(1 - p) - logBeta(a, b));
  }
  function logChoose(n, k) {
    if (k < 0 || k > n) return -Infinity;
    return logGamma(n + 1) - logGamma(k + 1) - logGamma(n - k + 1);
  }
  function binomSample(N, pr) {
    var c = 0;
    for (var i = 0; i < N; i++) if (Math.random() < pr) c++;
    return c;
  }
  function grid(N) { var xs = []; for (var i = 0; i <= N; i++) xs.push(i / N); return xs; }

  // ---------- symlog scale on p in [0,1] (linear below `t`, log above) ----------
  // fwd: p -> [0,1] screen fraction; inv: [0,1] -> p. C1-continuous at p=t.
  // This stretches the low-prevalence region, where pooled testing matters most.
  function makeScale(mode, t) {
    if (mode === 'symlog') {
      var Tmax = 1 + Math.log(1 / t);
      return {
        mode: mode, t: t,
        fwd: function (p) { return (p <= t ? p / t : 1 + Math.log(p / t)) / Tmax; },
        inv: function (u) { var uu = u * Tmax; return uu <= 1 ? uu * t : t * Math.exp(uu - 1); }
      };
    }
    return { mode: 'linear', fwd: function (p) { return p; }, inv: function (u) { return u; } };
  }
  var PX = makeScale('symlog', 0.01);           // shared p-axis + p-slider scale
  var PTICKS = [{ v: 0, label: '0' }, { v: 0.01, label: '.01' }, { v: 0.1, label: '.1' },
                { v: 0.5, label: '.5' }, { v: 1, label: '1' }];
  // sample p uniformly in transformed space so curves stay smooth after fwd()
  function scaleGrid(N, sx) { var xs = []; for (var i = 0; i <= N; i++) xs.push(sx.inv(i / N)); return xs; }
  function interp(xs, ys, x) {
    if (x <= xs[0]) return ys[0];
    if (x >= xs[xs.length - 1]) return ys[ys.length - 1];
    for (var i = 1; i < xs.length; i++) {
      if (xs[i] >= x) {
        var tt = (x - xs[i - 1]) / Math.max(xs[i] - xs[i - 1], 1e-12);
        return ys[i - 1] + tt * (ys[i] - ys[i - 1]);
      }
    }
    return ys[ys.length - 1];
  }
  function fmtP(p) { return p >= 0.1 ? p.toFixed(2) : p.toFixed(3); }

  // ---------- palette (reads CSS vars, so it is light/dark aware) ----------
  function palette(el) {
    var cs = getComputedStyle(el), g = function (v) { return cs.getPropertyValue(v).trim(); };
    return {
      surface: g('--rb-surface'), ink: g('--rb-ink'), sec: g('--rb-sec'),
      muted: g('--rb-muted'), axis: g('--rb-axis'), s1: g('--rb-s1'),
      s2: g('--rb-s2'), tent: g('--rb-tent')
    };
  }

  // ---------- generic canvas plot ----------
  function plot(cv, root, spec) {
    var dpr = window.devicePixelRatio || 1;
    var W = cv.clientWidth || 560, H = cv.clientHeight || 280;
    cv.width = Math.round(W * dpr); cv.height = Math.round(H * dpr);
    var ctx = cv.getContext('2d'); ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    var pal = palette(root);
    ctx.clearRect(0, 0, W, H);

    var m = { l: 48, r: 16, t: 14, b: 36 };
    var xL = m.l, xR = W - m.r, yB = H - m.b, yT = m.t;
    var xmin = spec.xmin != null ? spec.xmin : 0, xmax = spec.xmax != null ? spec.xmax : 1;
    var ymax = spec.ymax, ymin = spec.ymin;
    if (ymax == null || ymin == null) {
      var mx = -Infinity, mn = Infinity;
      spec.curves.forEach(function (c) {
        c.ys.forEach(function (v) { if (isFinite(v)) { if (v > mx) mx = v; if (v < mn) mn = v; } });
      });
      if (!isFinite(mx)) mx = 1; if (!isFinite(mn)) mn = 0;
      if (ymax == null) ymax = mx * 1.12;
      if (ymin == null) ymin = Math.min(0, mn * 1.12);
    }
    if (ymax <= ymin) ymax = ymin + 1;
    var sx = spec.xscale;
    var X = sx
      ? function (x) { return xL + sx.fwd(x) * (xR - xL); }
      : function (x) { return xL + (x - xmin) / (xmax - xmin) * (xR - xL); };
    var Y = function (y) { return yB - (y - ymin) / (ymax - ymin) * (yB - yT); };

    // CI shaded band under first curve, clipped to [xlo, xhi]
    if (spec.ci) {
      var b = spec.ci;
      ctx.beginPath(); var started = false, lastX = null;
      for (var i = 0; i < b.xs.length; i++) {
        if (b.xs[i] < b.xlo || b.xs[i] > b.xhi) continue;
        var px = X(b.xs[i]), py = Y(Math.max(b.ys[i], 0));
        if (!started) { ctx.moveTo(px, Y(0)); ctx.lineTo(px, py); started = true; }
        else ctx.lineTo(px, py);
        lastX = px;
      }
      if (started) { ctx.lineTo(lastX, Y(0)); ctx.closePath(); ctx.fillStyle = b.color || pal.tent; ctx.fill(); }
    }

    // zero line when the range spans negatives
    if (ymin < 0) {
      ctx.strokeStyle = pal.axis; ctx.globalAlpha = 0.7; ctx.lineWidth = 1;
      ctx.beginPath(); ctx.moveTo(xL, Y(0)); ctx.lineTo(xR, Y(0)); ctx.stroke(); ctx.globalAlpha = 1;
    }

    // axes
    ctx.strokeStyle = pal.axis; ctx.lineWidth = 1.25;
    ctx.beginPath(); ctx.moveTo(xL, yB); ctx.lineTo(xR, yB); ctx.moveTo(xL, yT); ctx.lineTo(xL, yB); ctx.stroke();

    // vertical reference lines
    (spec.vlines || []).forEach(function (v) {
      ctx.strokeStyle = v.color || pal.muted; ctx.lineWidth = 1;
      ctx.setLineDash(v.dash || [4, 4]);
      ctx.beginPath(); ctx.moveTo(X(v.x), yT); ctx.lineTo(X(v.x), yB); ctx.stroke();
      ctx.setLineDash([]);
    });

    // horizontal reference lines
    (spec.hlines || []).forEach(function (h) {
      ctx.strokeStyle = h.color || pal.muted; ctx.lineWidth = 1;
      ctx.setLineDash(h.dash || [5, 4]);
      ctx.beginPath(); ctx.moveTo(xL, Y(h.y)); ctx.lineTo(xR, Y(h.y)); ctx.stroke();
      ctx.setLineDash([]);
    });

    // curves
    spec.curves.forEach(function (c) {
      ctx.strokeStyle = c.color; ctx.lineWidth = c.width || 2;
      ctx.globalAlpha = c.opacity != null ? c.opacity : 1;
      if (c.dash) ctx.setLineDash(c.dash);
      ctx.beginPath(); var st = false;
      for (var i = 0; i < c.xs.length; i++) {
        var v = c.ys[i]; if (!isFinite(v)) continue;
        var px = X(c.xs[i]), py = Y(v);
        if (!st) { ctx.moveTo(px, py); st = true; } else ctx.lineTo(px, py);
      }
      ctx.stroke(); ctx.setLineDash([]); ctx.globalAlpha = 1;
    });

    // markers
    (spec.markers || []).forEach(function (mk) {
      ctx.fillStyle = mk.color || pal.ink; ctx.strokeStyle = pal.surface; ctx.lineWidth = 2;
      ctx.beginPath(); ctx.arc(X(mk.x), Y(mk.y), 4.5, 0, 2 * Math.PI); ctx.fill(); ctx.stroke();
    });

    // direct series labels (used instead of a legend for the ordered q family)
    (spec.labels || []).forEach(function (lb) {
      ctx.fillStyle = lb.color || pal.ink;
      ctx.font = '600 12px system-ui,-apple-system,sans-serif';
      ctx.textAlign = lb.align || 'left'; ctx.textBaseline = 'middle';
      ctx.fillText(lb.text, X(lb.x), Y(lb.y));
    });

    // ticks + labels
    ctx.fillStyle = pal.muted; ctx.font = '11px system-ui,-apple-system,sans-serif';
    ctx.textAlign = 'center'; ctx.textBaseline = 'top';
    var xticks = spec.xticks || [{ v: 0, label: '0' }, { v: 0.5, label: '0.5' }, { v: 1, label: '1' }];
    xticks.forEach(function (t) { ctx.fillText(t.label, X(t.v), yB + 8); });
    if (spec.yticks) {
      ctx.textAlign = 'right'; ctx.textBaseline = 'middle';
      spec.yticks.forEach(function (t) { ctx.fillText(t.label, xL - 7, Y(t.v)); });
    }

    ctx.save();
    ctx.fillStyle = pal.sec; ctx.font = '12px system-ui,-apple-system,sans-serif';
    ctx.textAlign = 'center'; ctx.textBaseline = 'alphabetic';
    if (spec.xlabel) ctx.fillText(spec.xlabel, (xL + xR) / 2, H - 6);
    ctx.translate(13, (yT + yB) / 2); ctx.rotate(-Math.PI / 2); ctx.textBaseline = 'middle';
    if (spec.ylabel) ctx.fillText(spec.ylabel, 0, 0);
    ctx.restore();
  }

  // ---------- helpers to grab elements ----------
  function $(id) { return document.getElementById(id); }

  var renders = [];
  function register(cv, renderFn) {
    if (!cv) return;
    renderFn();
    if (window.ResizeObserver) { new ResizeObserver(renderFn).observe(cv); }
    renders.push(renderFn);
  }

  // =====================================================================
  // WIDGET 1 — pooling intuition toy
  // =====================================================================
  (function () {
    var root = $('pv-pool'); if (!root) return;
    var cv = $('pool-canvas');
    var chipStates = [];

    function resampleChips(q, p) {
      chipStates = [];
      for (var i = 0; i < q; i++) chipStates.push(Math.random() < p);
    }
    function drawChips() {
      var box = $('pool-chips'); box.innerHTML = '';
      var anyPos = false;
      for (var i = 0; i < chipStates.length; i++) {
        var d = document.createElement('div');
        d.className = 'pv-chip' + (chipStates[i] ? ' pos' : '');
        box.appendChild(d);
        if (chipStates[i]) anyPos = true;
      }
      $('pool-result').textContent = anyPos ? 'POSITIVE' : 'negative';
      $('pool-result').style.color = anyPos ? 'var(--rb-s2)' : 'var(--rb-muted)';
    }
    function poolP() { return Math.min(Math.max(PX.inv(parseFloat($('pool-p').value)), 0.002), 0.998); }
    function render() {
      var p = poolP(), q = parseInt($('pool-q').value, 10);
      $('pool-p-out').textContent = fmtP(p);
      $('pool-q-out').textContent = q;
      var pi = 1 - Math.pow(1 - p, q);
      $('pool-pi').textContent = pi.toFixed(3);
      if (chipStates.length !== q) resampleChips(q, p);
      drawChips();
      var xs = scaleGrid(300, PX), ys = xs.map(function (x) { return 1 - Math.pow(1 - x, q); });
      plot(cv, root, {
        curves: [{ xs: xs, ys: ys, color: palette(root).s2, width: 2.5 }],
        ymin: 0, ymax: 1.05, xscale: PX, xticks: PTICKS,
        vlines: [{ x: p, color: palette(root).muted }],
        markers: [{ x: p, y: pi }],
        xlabel: 'true prevalence  p (symlog)', ylabel: 'Pr(pooled test positive)'
      });
    }
    var psl = $('pool-p'); var poolDefault = parseFloat(psl.value);
    psl.min = 0; psl.max = 1; psl.step = 0.005; psl.value = PX.fwd(poolDefault);
    psl.addEventListener('input', function () { resampleChips(parseInt($('pool-q').value, 10), poolP()); render(); });
    $('pool-q').addEventListener('input', function () { resampleChips(parseInt($('pool-q').value, 10), poolP()); render(); });
    $('pool-resample').addEventListener('click', function () {
      resampleChips(parseInt($('pool-q').value, 10), poolP()); render();
    });
    register(cv, render);
  })();

  // =====================================================================
  // WIDGET 2 — posterior playground (grid evaluation, stable)
  // =====================================================================
  (function () {
    var root = $('pv-play'); if (!root) return;
    var cv = $('pg-canvas');
    var N = 400;
    var data = { y: 0, z: 0 };

    function safeTerm(count, val) { // count * log(val), with 0*log(0)=0
      if (count === 0) return 0;
      if (val <= 0) return -Infinity;
      return count * Math.log(val);
    }
    function rates(p, q, se, sp) {
      var a = se * p + (1 - sp) * (1 - p);            // indiv positive prob
      var pi = 1 - Math.pow(1 - p, q);
      var b = se * pi + (1 - sp) * (1 - pi);          // pooled positive prob
      return { a: a, b: b };
    }
    function readModel() {
      return {
        m: parseInt($('pg-m').value, 10), n: parseInt($('pg-n').value, 10),
        q: parseInt($('pg-q').value, 10),
        P: Math.min(Math.max(PX.inv(parseFloat($('pg-P').value)), 0.002), 0.998),
        se: parseFloat($('pg-se').value), sp: parseFloat($('pg-sp').value)
      };
    }
    function expectedData(mdl) {
      var r = rates(mdl.P, mdl.q, mdl.se, mdl.sp);
      data.y = Math.round(mdl.m * r.a);
      data.z = Math.round(mdl.n * r.b);
    }
    function resampleData(mdl) {
      var r = rates(mdl.P, mdl.q, mdl.se, mdl.sp);
      data.y = binomSample(mdl.m, r.a);
      data.z = binomSample(mdl.n, r.b);
    }

    function posterior(mdl) {
      // grid is uniform in transformed space, so intervals in p are non-uniform:
      // every trapezoid uses its own width dp = xs[i] - xs[i-1].
      var xs = scaleGrid(N, PX), us = [], i;
      for (i = 0; i <= N; i++) {
        var p = Math.min(Math.max(xs[i], 1e-9), 1 - 1e-9);
        var r = rates(p, mdl.q, mdl.se, mdl.sp);
        var lu = safeTerm(data.y, r.a) + safeTerm(mdl.m - data.y, 1 - r.a)
               + safeTerm(data.z, r.b) + safeTerm(mdl.n - data.z, 1 - r.b);
        us.push(lu);
      }
      var lmax = -Infinity;
      for (i = 0; i <= N; i++) if (us[i] > lmax) lmax = us[i];
      var u = us.map(function (l) { return isFinite(l) ? Math.exp(l - lmax) : 0; });
      // trapezoid normalization + mean + CDF (variable spacing)
      var Z = 0, mean = 0, i2, dp;
      for (i2 = 1; i2 <= N; i2++) {
        dp = xs[i2] - xs[i2 - 1];
        Z += 0.5 * (u[i2] + u[i2 - 1]) * dp;
        mean += 0.5 * (xs[i2] * u[i2] + xs[i2 - 1] * u[i2 - 1]) * dp;
      }
      var dens = u.map(function (v) { return v / Z; });
      mean = mean / Z;
      // CDF and quantiles
      var cdf = [0], acc = 0;
      for (i2 = 1; i2 <= N; i2++) { acc += 0.5 * (dens[i2] + dens[i2 - 1]) * (xs[i2] - xs[i2 - 1]); cdf.push(acc); }
      function quantile(qq) {
        for (var k = 1; k <= N; k++) {
          if (cdf[k] >= qq) {
            var t = (qq - cdf[k - 1]) / Math.max(cdf[k] - cdf[k - 1], 1e-12);
            return xs[k - 1] + t * (xs[k] - xs[k - 1]);
          }
        }
        return 1;
      }
      return { xs: xs, dens: dens, mean: mean, lo: quantile(0.025), hi: quantile(0.975) };
    }

    function render() {
      var mdl = readModel();
      if (data.y > mdl.m) data.y = mdl.m;
      if (data.z > mdl.n) data.z = mdl.n;
      $('pg-m-out').textContent = mdl.m; $('pg-n-out').textContent = mdl.n;
      $('pg-q-out').textContent = mdl.q; $('pg-P-out').textContent = fmtP(mdl.P);
      $('pg-se-out').textContent = mdl.se.toFixed(2); $('pg-sp-out').textContent = mdl.sp.toFixed(2);
      $('pg-y').textContent = data.y; $('pg-z').textContent = data.z;

      var post = posterior(mdl);
      $('pg-mean').textContent = post.mean.toFixed(3);
      $('pg-ci').textContent = '[' + post.lo.toFixed(3) + ', ' + post.hi.toFixed(3) + ']';
      var meanDens = interp(post.xs, post.dens, post.mean);
      plot(cv, root, {
        curves: [{ xs: post.xs, ys: post.dens, color: palette(root).s1, width: 2.5 }],
        ymin: 0, xscale: PX, xticks: PTICKS,
        ci: { xs: post.xs, ys: post.dens, xlo: post.lo, xhi: post.hi, color: palette(root).tent },
        vlines: [{ x: mdl.P, color: palette(root).s2, dash: [5, 4] }],
        markers: [{ x: post.mean, y: meanDens }],
        xlabel: 'prevalence  p (symlog)', ylabel: 'posterior density'
      });
    }
    // default prevalence comes from the input's HTML value= attribute (in real p),
    // then we switch the slider into transformed symlog units.
    var Psl = $('pg-P'); var pDefault = parseFloat(Psl.value);
    Psl.min = 0; Psl.max = 1; Psl.step = 0.005; Psl.value = PX.fwd(pDefault);
    ['pg-m', 'pg-n', 'pg-q', 'pg-P', 'pg-se', 'pg-sp'].forEach(function (id) {
      $(id).addEventListener('input', function () { expectedData(readModel()); render(); });
    });
    $('pg-resample').addEventListener('click', function () { resampleData(readModel()); render(); });

    expectedData(readModel());
    register(cv, render);
  })();

  // =====================================================================
  // WIDGET 3 — sum-of-betas reveal (closed form, small counts)
  // =====================================================================
  (function () {
    var root = $('pv-betas'); if (!root) return;
    var cv = $('b-canvas');
    var alpha = 1, beta = 1, N = 300;

    function render() {
      var y = parseInt($('b-y').value, 10), m = parseInt($('b-m').value, 10);
      var z = parseInt($('b-z').value, 10), n = parseInt($('b-n').value, 10);
      var q = parseInt($('b-q').value, 10);
      if (y > m) { y = m; $('b-y').value = y; }
      if (z > n) { z = n; $('b-z').value = z; }
      $('b-y-out').textContent = y; $('b-m-out').textContent = m;
      $('b-z-out').textContent = z; $('b-n-out').textContent = n; $('b-q-out').textContent = q;

      var gamma = y + alpha;
      var delta = m - y + beta + q * n - q * z;

      // signed weights w_i = C(z,i)(-1)^i B(gamma, delta+qi); components Beta(gamma, delta+qi)
      var comps = [], S = 0;
      for (var i = 0; i <= z; i++) {
        var A = gamma, B = delta + q * i;
        var w = Math.exp(logChoose(z, i) + logBeta(A, B)) * (i % 2 === 0 ? 1 : -1);
        comps.push({ A: A, B: B, w: w });
        S += w;
      }
      var xs = scaleGrid(N, PX);
      var pal = palette(root);
      var curves = [];
      // components (signed, normalized)
      comps.forEach(function (c, i) {
        var wn = c.w / S;
        var ys = xs.map(function (p) { return wn * betaPdf(p, c.A, c.B); });
        curves.push({ xs: xs, ys: ys, color: i % 2 === 0 ? pal.s1 : pal.s2, width: 1.4, opacity: 0.55 });
      });
      // the posterior sum
      var sum = xs.map(function (p, k) {
        var s = 0; comps.forEach(function (c) { s += (c.w / S) * betaPdf(p, c.A, c.B); }); return s;
      });
      // closest single Beta by method of moments
      var mu1 = 0, mu2 = 0;
      comps.forEach(function (c) {
        var wn = c.w / S, ab = c.A + c.B;
        mu1 += wn * (c.A / ab);
        mu2 += wn * (c.A * (c.A + 1) / (ab * (ab + 1)));
      });
      var v = mu2 - mu1 * mu1;
      if (v > 1e-9 && mu1 > 0 && mu1 < 1) {
        var common = mu1 * (1 - mu1) / v - 1;
        var fa = mu1 * common, fb = (1 - mu1) * common;
        if (fa > 0 && fb > 0) {
          var fit = xs.map(function (p) { return betaPdf(p, fa, fb); });
          curves.push({ xs: xs, ys: fit, color: pal.muted, width: 1.6, dash: [5, 4] });
        }
      }
      curves.push({ xs: xs, ys: sum, color: pal.ink, width: 2.8 });

      plot(cv, root, {
        curves: curves, xscale: PX, xticks: PTICKS,
        xlabel: 'prevalence  p (symlog)', ylabel: 'density'
      });
    }
    ['b-y', 'b-m', 'b-z', 'b-n', 'b-q'].forEach(function (id) {
      $(id).addEventListener('input', render);
    });
    register(cv, render);
  })();

  // =====================================================================
  // WIDGET 4 — relative Fisher information, pooled vs individual, across q
  // =====================================================================
  (function () {
    var root = $('pv-info'); if (!root) return;
    var cv = $('info-canvas');
    var N = 320;
    // I_Y/I_X = q^2 p (1-p)^(q-1) / (1 - (1-p)^q);  -> q as p->0, -> 0 as p->1
    function ratio(p, q) { return q * q * p * Math.pow(1 - p, q - 1) / (1 - Math.pow(1 - p, q)); }
    // sequential blue ramp (light -> dark = small -> large q); site is light-only
    var QS = [{ q: 2, c: '#9ec3ea' }, { q: 4, c: '#5b9be0' }, { q: 6, c: '#2a78d6' }, { q: 8, c: '#17539e' }];
    function render() {
      var pal = palette(root);
      var xs = scaleGrid(N, PX);
      var curves = [], labels = [];
      QS.forEach(function (o) {
        var ys = xs.map(function (p) { return ratio(Math.min(Math.max(p, 1e-9), 1 - 1e-9), o.q); });
        curves.push({ xs: xs, ys: ys, color: o.c, width: 2.4 });
        // label at the left plateau, where each curve sits near height q (well separated)
        labels.push({ x: 0.013, y: ratio(0.013, o.q) + 0.28, text: 'q = ' + o.q, color: o.c, align: 'left' });
      });
      labels.push({ x: 0.02, y: .75, text: 'q = 1  (individual test)', color: pal.muted, align: 'right' });
      plot(cv, root, {
        curves: curves, xscale: PX, xticks: PTICKS,
        ymin: 0, ymax: 8.7,
        yticks: [{ v: 0, label: '0' }, { v: 1, label: '1' }, { v: 2, label: '2' },
                 { v: 4, label: '4' }, { v: 6, label: '6' }, { v: 8, label: '8' }],
        hlines: [{ y: 1, color: pal.muted, dash: [5, 4] }],
        labels: labels,
        xlabel: 'prevalence  p (symlog)', ylabel: 'info ratio  (pooled ÷ individual)'
      });
    }
    register(cv, render);
  })();

  // ---------- re-render everything on theme flip ----------
  if (window.matchMedia) {
    var mq = window.matchMedia('(prefers-color-scheme: dark)');
    var relay = function () { renders.forEach(function (r) { r(); }); };
    if (mq.addEventListener) mq.addEventListener('change', relay);
    else if (mq.addListener) mq.addListener(relay);
  }
})();
</script>



