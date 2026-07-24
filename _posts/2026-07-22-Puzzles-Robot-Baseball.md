---
layout: post  
title: "Solving Jane Street's Robot Baseball Puzzle: Markov Games and Golden Section Search"
date: 2026-07-22 17:00:00 -0400  
categories: blog  
tags: [game theory, probability, puzzles]
excerpt: Solving Jane Street's Robot Baseball puzzle with Markov game analysis and golden section search, an enjoyable mix of symbolic math and numerical methods.
image: /images/puz2_R.png
---


* TOC
{:toc}




Recently, I have been enjoying solving Jane Street's backlog of [puzzles](https://www.janestreet.com/puzzles). Last night, I solved [Robot Baseball](https://www.janestreet.com/puzzles/robot-baseball-index/) from October 2025. This post is my write-up of the math I learned while solving it, so if you haven't already, take a second to read the puzzle!

In brief, Robot Baseball is a [Markov game](https://en.wikipedia.org/wiki/Stochastic_game). Its states are numbers of balls and strikes. The game ends when balls=4 or strikes=3. To find the [Nash equilibria](https://en.wikipedia.org/wiki/Nash_equilibrium) at each state, we need to first model the terminal states and then use backward induction to solve each [subgame equilibrium](https://en.wikipedia.org/wiki/Subgame_perfect_equilibrium) until we have found an equilibrium for each possible state. 

The catch is that if the pitcher throws a strike and the batter swings, the batter hits with probability $$p$$ and misses with probability $$1 - p$$. We want to optimize $$p$$ to maximize our odds of reaching $$(\text{balls}=3, \text{strikes}=2)$$. The players' payoffs at each step and their equilibrium strategies are therefore functions of $$p$$. This is straightforward symbolically when we are calculating the equilibrium for $$(\text{balls}=3, \text{strikes}=2)$$, but over the many inductive steps back to the initial state of the game, those symbolic expressions blow up into an ungodly hard-to-compute-precisely large polynomials of $$p$$.

The key is to realize that 

1) For a given $$p$$, the equilibria for the full game can be computed very quickly 

2) $$\Pr(\text{balls}=3, \text{strikes}=2)$$ when viewed as a function of $$p$$ has a single maximum value

These two things mean that we can use a black box search algorithm like golden section search to compute the $$p$$ that maximizes $$\Pr(\text{balls}=3, \text{strikes}=2)$$ to arbitrary precision.

Now, let's go through the derivations and solution in detail.

## Modeling a Single-Turn Zero-Sum Game

Let's forget everything about robot baseball for a minute and think about a very simple game. 

Player 1 has a choice to select Top or Bottom. They select Top with some probability $$p$$ and Bottom with probability $$1-p$$.

Player 2 has a choice to select Left or Right. They select Left with some probability $$q$$ and Right with probability $$1-q$$.

Let's say there are 4 outcomes possible depending on the combination of actions the players take. The players choose actions simultaneously.

Let's say that this is a zero-sum game, meaning that for a given outcome Player 1's gain is equivalent to Player 2's loss. We'll put those payoffs in terms of gains or losses for Player 1 and leave them as variables:  $$a, b, c, d$$. Remember that a benefit of $$a$$ to Player 1 is a benefit of $$-a$$ to Player 2.

We'll also assume that both players have total knowledge of the game's payoff matrix.

We can record this setup for the game in a payoff matrix.

<table class="payoff-matrix">
  <tr>
    <td class="pm-group"></td>
    <td class="pm-group"></td>
    <th class="pm-group" colspan="2">Player 2</th>
  </tr>
  <tr>
    <td class="pm-group"></td>
    <td class="pm-group"></td>
    <th class="pm-act">Left, \(q\)</th>
    <th class="pm-act">Right, \(1-q\)</th>
  </tr>
  <tr>
    <th class="pm-group rot" rowspan="2"><span>Player 1</span></th>
    <th class="pm-act">Top, \(p\)</th>
    <td class="pm-pay">\(a\)</td>
    <td class="pm-pay">\(b\)</td>
  </tr>
  <tr>
    <th class="pm-act">Bottom, \(1-p\)</th>
    <td class="pm-pay">\(c\)</td>
    <td class="pm-pay">\(d\)</td>
  </tr>
</table>

<style>
.payoff-matrix {
  --pm-line: #d3d2c8;
  --pm-head: rgba(137, 135, 129, 0.12);
  --pm-cell: #fcfcfb;
  --pm-ink: #1a1a19;
  --pm-muted: #6f6e69;
  border-collapse: collapse;
  margin: 1.6em auto;
  color: var(--pm-ink);
  font-variant-numeric: tabular-nums;
  line-height: 1.2;
}
.payoff-matrix th, .payoff-matrix td { padding: 0.6em 1em; text-align: center; }
/* outer "who is choosing" labels — muted, borderless */
.payoff-matrix .pm-group {
  border: none; background: none;
  color: var(--pm-muted); font-weight: 600;
  font-size: 0.78em; letter-spacing: 0.05em; text-transform: uppercase;
}
/* rotated left-hand label */
.payoff-matrix .rot span {
  writing-mode: vertical-rl; transform: rotate(180deg); display: inline-block;
}
/* action headers */
.payoff-matrix .pm-act {
  border: 1px solid var(--pm-line); background: var(--pm-head); font-weight: 600;
}
/* payoff cells (values are Player 1 / the batter's expected score) */
.payoff-matrix .pm-pay {
  border: 1px solid var(--pm-line); background: var(--pm-cell); min-width: 5.5em;
}
</style>

Now, we want to model the expected payoffs for each player across the set of strategies they can choose to identify their optimal strategies. 

For shorthand, we'll use $$\operatorname{E}[L]$$ for Player 1's expected score if Player 2 chooses Left and $$\operatorname{E}[R]$$ for Player 1's expected score if Player 2 chooses Right.

$$
\operatorname{E}[L] = pa + (1-p)c = c + (a-c)p
$$

$$
\operatorname{E}[R] = pb + (1-p)d = d + (b-d)p
$$

If Player 1 picks $$p$$ so that $$\operatorname{E}[L] > \operatorname{E}[R]$$, Player 2 could win more by picking Right, and vice versa. To maximize their score against any opponent, including one that knows their strategy, Player 1 wants to maximize $$\min(\operatorname{E}[L], \operatorname{E}[R])$$. 

We can graph both expectations as functions of $$p$$:

<figure class="rb-fig">
  <svg viewBox="0 0 640 420" role="img" aria-labelledby="rb-title rb-desc" xmlns="http://www.w3.org/2000/svg">
    <title id="rb-title">Player 1's payoff versus p</title>
    <desc id="rb-desc">Two straight lines, E[L] and E[R], plotted against p. E[L] rises and E[R] falls, crossing at p-star. The lower of the two lines forms a tent shape whose peak, at the crossing, is the highest payoff Player 1 can guarantee.</desc>

    <!-- < min(E[L], E[R]) -->
    <polygon points="56,315 352.4,178.2 550,201 550,372 56,372" fill="var(--rb-tent)"/>

    <!-- axes -->
    <path d="M56 372 L550 372" stroke="var(--rb-axis)" stroke-width="1.5"/>
    <path d="M56 30 L56 372" stroke="var(--rb-axis)" stroke-width="1.5"/>

    <!-- droplines to p* and V -->
    <path d="M352.4 178.2 L352.4 372" stroke="var(--rb-muted)" stroke-width="1" stroke-dasharray="4 4"/>
    <path d="M352.4 178.2 L56 178.2" stroke="var(--rb-muted)" stroke-width="1" stroke-dasharray="4 4"/>

    <!-- max(E[L], E[R]) -->
    <path d="M352.4 178.2 L550 87" stroke="var(--rb-s1)" stroke-width="2" stroke-opacity="0.3" fill="none"/>
    <path d="M56 144 L352.4 178.2" stroke="var(--rb-s2)" stroke-width="2" stroke-opacity="0.3" fill="none"/>

    <!-- min(E[L], E[R]) -->
    <path d="M56 315 L352.4 178.2" stroke="var(--rb-s1)" stroke-width="2.5" fill="none"/>
    <path d="M352.4 178.2 L550 201" stroke="var(--rb-s2)" stroke-width="2.5" fill="none"/>

    <!-- peak -->
    <path d="M352.4 70 L352.4 170" stroke="var(--rb-muted)" stroke-width="1" stroke-dasharray="3 3"/>
    <circle cx="352.4" cy="178.2" r="5" fill="var(--rb-ink)" stroke="var(--rb-surface)" stroke-width="2"/>

    <!-- series labels -->
    <text x="556" y="90" fill="var(--rb-s1)" font-size="14" font-weight="600">E[L]</text>
    <text x="556" y="205" fill="var(--rb-s2)" font-size="14" font-weight="600">E[R]</text>


    <!-- region labels -->
    <text x="200" y="352" fill="var(--rb-muted)" font-size="12" text-anchor="middle">Player 2 picks Left</text>
    <text x="458" y="352" fill="var(--rb-muted)" font-size="12" text-anchor="middle">Player 2 picks Right</text>

    <!-- tick labels -->
    <text x="56"  y="390" fill="var(--rb-muted)" font-size="11" text-anchor="middle">0</text>
    <text x="352.4" y="390" fill="var(--rb-muted)" font-size="11" text-anchor="middle">p*</text>
    <text x="550" y="390" fill="var(--rb-muted)" font-size="11" text-anchor="middle">1</text>
    <text x="48"  y="182" fill="var(--rb-muted)" font-size="11" text-anchor="end">V</text>
    <text x="48"  y="376" fill="var(--rb-muted)" font-size="11" text-anchor="end">0</text>

    <!-- axis titles -->
    <text x="303" y="410" fill="var(--rb-sec)" font-size="12" text-anchor="middle">p</text>
    <text x="18" y="201" fill="var(--rb-sec)" font-size="12" text-anchor="middle" transform="rotate(-90 18 201)">Player 1's expected payoff</text>
  </svg>
</figure>

<style>
.rb-fig {
  margin: 1.5em auto; max-width: 640px; text-align: center;
  font-family: system-ui, -apple-system, "Segoe UI", sans-serif;
  --rb-surface: #fcfcfb; --rb-ink: #0b0b0b; --rb-sec: #52514e; --rb-muted: #898781;
  --rb-axis: #c3c2b7; --rb-s1: #2a78d6; --rb-s2: #eb6834;
  --rb-tent: rgba(137,135,129,0.15);
}
.rb-fig svg { width: 100%; height: auto; overflow: visible; }
.rb-fig figcaption { font-size: 0.85em; color: var(--rb-sec); margin-top: 0.6em; line-height: 1.4; }
@media (prefers-color-scheme: dark) {
  .rb-fig {
    --rb-surface: #1a1a19; --rb-ink: #ffffff; --rb-sec: #c3c2b7; --rb-muted: #898781;
    --rb-axis: #383835; --rb-s1: #3987e5; --rb-s2: #d95926;
    --rb-tent: rgba(137,135,129,0.22);
  }
}
</style>



$$\min(\operatorname{E}[L], \operatorname{E}[R])$$ is concave, so it will have some maximum within $$[0,1]$$. If that max $$\in\{0, 1\}$$, then Player 1 will just always pick one of their options because it will always be better.

Otherwise, that  $$\max(\min(\operatorname{E}[L], \operatorname{E}[R]))$$ will occur at the intersection of $$\operatorname{E}[L] = \operatorname{E}[R]$$. Therefore, Player 1 selects $$p$$ to meet that condition:

$$\operatorname{E}[L] = \operatorname{E}[R]$$

$$c + (a-c)p = d + (b-d)p $$

$$ c - d = (b-d-a+c)p$$

$$p = \frac{c-d}{b-d-a+c}$$

You can do the same calculation for $$q$$. 

These strategies $$p$$ and $$q$$ are this game's Nash equilibrium.


## Modeling a Simultaneous-Move Markov Game

Markov games are repeated games where the players progress through game states where some or all of the progression probabilities are stochastic. Robot Baseball is a simultaneous-move Markov game.

Let's call the game state where $$b$$ is the number of balls and $$s$$ is the number of strikes $$(b,s)$$.

Let's call the expected value of a state of the game for the batter $$V(b,s)$$. This is a zero-sum game, so the expected value for the pitcher would be $$-V(b,s)$$.

We know that $$V(4,s) = 1$$ and $$V(b,3) = 0$$ for any values of b or s. A home run has a value of 4. These are our terminal states.

To model this game, we will need to start from these known terminal states and work backwards via induction. First, let's set up our game's payoff table for an arbitrary state $$(b,s)$$. We will put the values of the next states in terms of our function V so this can be used as a recurrence relation. 

Remember from the question that we do not know $$p$$, which is the probability that StrikexSwing will net a home run.

Let's simplify our notation a bit by substituting $$X=V(b+1,s)$$ and $$Y=V(b,s+1)$$.

<table class="payoff-matrix">
  <tr>
    <td class="pm-group"></td>
    <td class="pm-group"></td>
    <th class="pm-group" colspan="2">Batter</th>
  </tr>
  <tr>
    <td class="pm-group"></td>
    <td class="pm-group"></td>
    <th class="pm-act">Ball, \(q\)</th>
    <th class="pm-act">Strike, \(1-q\)</th>
  </tr>
  <tr>
    <th class="pm-group rot" rowspan="2"><span>Pitcher</span></th>
    <th class="pm-act">Wait, \(r\)</th>
    <td class="pm-pay">\(X\)</td>
    <td class="pm-pay">\(Y\)</td>
  </tr>
  <tr>
    <th class="pm-act">Swing, \(1-r\)</th>
    <td class="pm-pay">\(Y\)</td>
    <td class="pm-pay">\(4p + (1-p)Y\)</td>
  </tr>
</table>



Let's find the Nash equilibrium for this state of the game.

$$V(b,s) = \operatorname{E}[\text{Wait}] = \operatorname{E}[\text{Swing}]$$

$$qX + (1-q)Y = qY + (1-q)[4p + (1-p)Y]$$

Rearrange to see that 

$$q = \frac{p(4-Y)}{X-Y+p(4-Y)}$$

and we can see by the symmetry of the payoff matrix that $$r = q$$. 

So, we can find a general form for $$V(b,s)$$:

$$V(b,s) = \operatorname{E}[\text{Wait}]$$

$$ = qX + (1-q)Y $$

$$ = \frac{X\big(Y+p(4-Y)\big) - Y^2}{X-2Y+4p+(1-p)Y} $$

Which is a recurrence relation that can combine this with our boundary values $$V(4,s) = 1$$ and $$V(b,3) = 0$$ to calculate any $$V(b,s)$$. This process is called Backward Induction.

Calculating $$V(b,s)$$ symbolically yields a a rational function in $$p$$ with degree-49 numerator and denominator with huge coefficients, but luckily we don't need to do that. Calculating the full table of values for $$V(b,s)$$ given some value for $$p$$ can be done very quickly, which will be enough for us.

Now, since we know $$q$$ and $$r$$ at each possible state of the game, we can calculate transition probabilities between states. Remember that $$q$$=$$r$$

At state $$(b,s)$$:

$$P(T_{n+1} \mid T_n = (b,s)) = \begin{cases} 
q^2 & \text{if } (b+1, s) \\ 
2q(1-q) + (1-p)(1-q)^2 & \text{if } (b, s+1) \\ 
p(1-q)^2 & \text{if } \text{Homer} 
\end{cases}$$

We want to maximize our odds of getting to $$(3,2)$$.

Call $$R(b,s)$$ the probability of reaching $$(3,2)$$ from $$(b,s)$$. 

$$R(3,2)=1$$ 

$$R(b,3) = R(4,s) = 0,$$

From reading off our transition table, we can see that

$$R(b,s) = q^2 R(b+1,s) + [2q-2q^2 + (1-p)(1-q)^2]R(b,s+1)$$

$$ = q^2 R(b+1,s) + [1-q^2 - p(1-q)^2]R(b,s+1)$$

I've skipped it here to simplify notation, but remember that $$q$$ is itself a function of $$(b,s)$$. So, we have another recurrence relation that we can combine with our recurrence relation for $$V(b,s)$$ to calculate $$R(0,0)$$. $$R(0,0)$$ is another insane rational function of $$p$$, but luckily again, we won't need to use that symbolic form. Given some $$p$$, we can quickly calculate $$R(0,0)$$.

## Visualizing the Value and Reach Probability as Functions of p

Before we move on to solving, I wanted to show off these curves!

![Plot of the batter's expected value V(b,s) at each ball-strike count as a function of the hit probability p](/images/puz2_V.png)

![Plot of R(b,s), the probability of reaching a full count from each game state, as a function of the hit probability p, showing a single clear maximum for R(0,0)](/images/puz2_R.png)

Look at that clean maximum for $$R(0,0)$$!

## Black Box Search and Solving the Puzzle

Now we can calculate $$R(0,0)$$ quickly for any value of $$p$$ and can see in the plot above that $$R(0,0)$$ has a clear single maximum in $$[0,1]$$. We can adapt Descartes' Rule of Signs to rigorously prove that $$R(0,0)$$ has a single maximum in $$[0,1]$$, but I will skip that.

Let's calculate $$\operatorname*{arg\,max}\limits_{p \in [0,1]}\ R(0,0)$$. 

We will use [golden section search](https://en.wikipedia.org/wiki/Golden-section_search), a blackbox (derivative free) optimization technique, because maximizing $$R(0,0)$$ would involve computing the derivative of an insane rational polynomial in $$p$$, which would cause a slew of precision issues that we don't need to deal with. 

This runs in less than a second. Here are our solutions to 10 decimal places:

$$\operatorname*{arg\,max}\limits_{p \in [0,1]}\ R(0,0) = 0.2269732325$$

$$\operatorname*{max}\limits_{p \in [0,1]}\ R(0,0) = 0.2959679934$$

Here is the search's trace, in case you're curious.

|N Evals|Lower Bound|Upper Bound|Best Guess max of R(0,0) |
|---|---|---|---|
| 0 | 0.000000000000 | 1.000000000000 | 0.247428935807 |
| 3 | 0.000000000000 | 0.618033988750 | 0.295748501269 |
| 4 | 0.000000000000 | 0.381966011250 | 0.295748501269 |
| 5 | 0.145898033750 | 0.381966011250 | 0.295748501269 |
| 6 | 0.145898033750 | 0.291796067501 | 0.295748501269 |
| 7 | 0.201626123751 | 0.291796067501 | 0.295748501269 |
| 8 | 0.201626123751 | 0.257354213752 | 0.295923558506 |
| 9 | 0.201626123751 | 0.236067977500 | 0.295923558506 |
| 10 | 0.214781741248 | 0.236067977500 | 0.295965502715 |
| 11 | 0.222912360003 | 0.236067977500 | 0.295965502715 |
| 12 | 0.222912360003 | 0.231042978759 | 0.295965543018 |
| 13 | 0.222912360003 | 0.227937358744 | 0.295965543018 |
| 14 | 0.224831738729 | 0.227937358744 | 0.295967861004 |
| 15 | 0.226017980019 | 0.227937358744 | 0.295967861004 |
| 16 | 0.226017980019 | 0.227204221308 | 0.295967861004 |
| 17 | 0.226471083872 | 0.227204221308 | 0.295967986922 |
| 18 | 0.226751117454 | 0.227204221308 | 0.295967986922 |
| 19 | 0.226751117454 | 0.227031151036 | 0.295967986922 |
| 20 | 0.226858080765 | 0.227031151036 | 0.295967993194 |
| 21 | 0.226924187726 | 0.227031151036 | 0.295967993194 |
| 22 | 0.226924187726 | 0.226990294687 | 0.295967993194 |
| 23 | 0.226949438338 | 0.226990294687 | 0.295967993369 |
| 24 | 0.226965044075 | 0.226990294687 | 0.295967993369 |
| 25 | 0.226965044075 | 0.226980649812 | 0.295967993369 |
| 26 | 0.226971004936 | 0.226980649812 | 0.295967993369 |
| 27 | 0.226971004936 | 0.226976965797 | 0.295967993374 |
| 28 | 0.226971004936 | 0.226974688951 | 0.295967993374 |
| 29 | 0.226972412104 | 0.226974688951 | 0.295967993374 |
| 30 | 0.226972412104 | 0.226973819273 | 0.295967993374 |
| 31 | 0.226972949595 | 0.226973819273 | 0.295967993374 |
| 32 | 0.226972949595 | 0.226973487085 | 0.295967993374 |
| 33 | 0.226973154898 | 0.226973487085 | 0.295967993374 |
| 34 | 0.226973154898 | 0.226973360201 | 0.295967993374 |
| 35 | 0.226973154898 | 0.226973281782 | 0.295967993374 |
| 36 | 0.226973203364 | 0.226973281782 | 0.295967993374 |
| 37 | 0.226973203364 | 0.226973251829 | 0.295967993374 |
| 38 | 0.226973221876 | 0.226973251829 | 0.295967993374 |
| 39 | 0.226973221876 | 0.226973240388 | 0.295967993374 |
| 40 | 0.226973228947 | 0.226973240388 | 0.295967993374 |
| 41 | 0.226973228947 | 0.226973236018 | 0.295967993374 |
| 42 | 0.226973231648 | 0.226973236018 | 0.295967993374 |
| 43 | 0.226973231648 | 0.226973234349 | 0.295967993374 |
| 44 | 0.226973231648 | 0.226973233317 | 0.295967993374 |
| 45 | 0.226973232285 | 0.226973233317 | 0.295967993374 |
| 46 | 0.226973232285 | 0.226973232923 | 0.295967993374 |
| 47 | 0.226973232285 | 0.226973232679 | 0.295967993374 |
| 48 | 0.226973232436 | 0.226973232679 | 0.295967993374 |
| 49 | 0.226973232436 | 0.226973232586 | 0.295967993374 |
| 50 | 0.226973232493 | 0.226973232586 | 0.295967993374 |
| 51 | 0.226973232493 | 0.226973232551 | 0.295967993374 |
| 52 | 0.226973232515 | 0.226973232551 | 0.295967993374 |
| 53 | 0.226973232529 | 0.226973232551 | 0.295967993374 |
| 54 | 0.226973232529 | 0.226973232542 | 0.295967993374 |


