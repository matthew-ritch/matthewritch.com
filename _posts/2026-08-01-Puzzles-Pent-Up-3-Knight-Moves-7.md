---
layout: post  
title: "Jane Street's ‘Pent-Up’ Frustration 3 / Knight Moves 7 Puzzle: Constraint Satisfaction and Backtracking"
date: 2026-08-01 17:00:00 -0400  
categories: blog  
tags: [puzzles]
excerpt: Solving Jane Street's ‘Pent-Up’ Frustration 3 / Knight Moves 7 puzzle with backtracking depth-first search.
image: /images/puzzles-pu3km7.png
---

* TOC
{:toc}

This post will take you through my solution of Jane Street's July 2026 Puzzle, [‘Pent-Up’ Frustration 3 / Knight Moves 7](https://www.janestreet.com/puzzles/pent-up-frustration-3-knight-moves-7-index/). As always, I recommend you try it out yourself before reading this article!

This was a fun one that is pretty straightforward once you grasp the rules. This is essentially a constraint satisfaction problem. We can find the solution sequence of knight's moves with some clever backtracking. 

## Setup

Those constraints are:

- Never leave the grid
- Never revisit a space
- Visit each tower
- One tower per pent(tetr)omino
- If our knight reaches a tile on one of the moves where it is supposed to "write down" its score, its score after that move must match the score on the tile

Let's set some notation. Let's identify the board's positions by tuples $$(x,y,z)$$, where $$x=0$$ on the far left column, $$y=0$$ on the bottom row, and $$z=0$$ if the knight is not on a tower or $$z=1$$ if the knight is on a tower. The knight's moves $$(dx\ dy\ dz)$$ are signed permutations of $$\{2,1,0\}$$, where $$dz$$ can only be $$+1$$, $$0$$, or $$-1$$.

We'll call the knight's score after $$i$$ moves $$S_i$$, the set of moves after which our knight writes down its score $$R = \{0, 3, 6, 9, 12, 15, 18, 18+k \dots 18+5k\}$$ for some $$k > 3$$, the score written down on position $$(x,y)$$ $$\operatorname{hint}(x,y)$$, and our position after $$i$$ moves $$(x_i, y_i, z_i)$$.

I think a crucial first insight to solving this problem is that the towers' positions are not a separate optimization problem from the knight's move. A proper sequence of knight's moves will _imply_ the proper tower positions. So, I think it is useful to translate the above constraints to:

- $$0 \le x \le 7$$, $$0 \le y \le 7$$, $$0 \le z \le 1$$
- Never revisit an $$(x,y)$$
- Exactly once for each region, occupy a position in that region with $$z=1$$
- For each move $$i$$, if $$i \in R$$, $$S_i = \operatorname{hint}(x_i, y_i)$$

<!-- graphic of the puzzle with score hints and region borders. We need to make our own because we are going to annotate it to illustrate the first few moves -->

{% include figures/js-july-2026/style.html %}

{% include figures/js-july-2026/board.html %}

## Early exploration

Once I grokked the constraints and movement, I took a few minutes to find which of the nearby number tiles could possibly be the knight's position after move 3. I found that the only way to get to a correct hint score was to start on a tower. The first 4 positions including our start must be:

| $$i$$ | Position | Score |
|---|---|---|
| 0 | $$(0,0,1)$$ | 0 |
| 1 | $$(2,1,1)$$ | 1 |
| 2 | $$(4,2,1)$$ | 3 |
| 3 | $$(6,2,0)$$ | 1 |

Let's look at that path on the board. Our moves are numbered in the top left corner, intermediate scores are shown in faint type, and towers are marked by triangles in the upper right corner.

<!-- graphic showing the path so far. -->

{% include figures/js-july-2026/opening.html %} 

Then, only using the possible moves from this position, you can enumerate the possible sequences of scores $$S_1$$ through $$S_6$$.

<!-- insert the list of possible scores: -->

{% include figures/js-july-2026/score-sequences.html %}

Which of these sequences have an $$S_6$$ that is in our set of hints?

<!-- Show the same list with invalid S_6 values crossed out -->

{% include figures/js-july-2026/score-sequences-filtered.html %}

Ok, so there is one option there. $$S_6$$ must be 16.
Now, what sequence of moves will get us there?

<!-- Show the list off possible moves -->

{% include figures/js-july-2026/candidates.html %}

Now, we see that we have three options for moves 4 through 6. Think about this like sudoku: we'll assume one is correct, and then run the exact same analysis of possible point sequences and legal moves to $$S_9$$. If there is no legal $$S_9$$ under our assumption for moves 4 through 6, then we know that assumption is wrong and we can eliminate that option. If there are multiple possible paths there, then we can repeat the same process recursively.

Doing that by hand would take forever, so luckily, there are algorithmic methods for solving this exact kind of problem.

## Backtracking: depth-first search with pruning

This puzzle is a [constraint satisfaction problem](https://en.wikipedia.org/wiki/Constraint_satisfaction_problem). We want to construct a set of moves that satisfy all of the rules. Imagine our knight's paths-so-far as nodes in a [tree](https://en.wikipedia.org/wiki/Tree_(graph_theory)). Each subsequent move takes our knight to a child node and eventually our puzzle's solution will be on a leaf.

The work I showed to get us to move 6 in the last section is a classic application of [backtracking](https://en.wikipedia.org/wiki/Backtracking), a common approach for this sort of problem. We will use [depth-first search](https://en.wikipedia.org/wiki/Depth-first_search) (DFS) to build a path of nodes while checking our list of constraints to eliminate paths that are in violation. If all of a node's child nodes have been eliminated, then that node will also be eliminated. We can thus "backtrack" to the next most recent possible parent node and continue our DFS from there.

## Solving the problem

Now, we encode our constraints and apply DFS. One final wrinkle is deciding a value for $$k$$. Remember that after move 18, the knight begins recording its score every $$k$$ moves for some $$k>3$$. That can be solved by picking some $$k$$, running the DFS, and seeing if it creates a constraint-satisfying path. That only works with $$k=7$$.

<!-- graphic of final board, with moves numbered in the top left and intermediate scores shown in faint type -->

{% include figures/js-july-2026/final-board.html %}

Once you have this path, just add up the scores on the tiles adjacent to the unvisited tiles to find the answer.