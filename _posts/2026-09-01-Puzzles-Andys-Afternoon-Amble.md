---
layout: post  
title: "Jane Street's Andy's Afternoon Amble Puzzle: Geometry and Random Walks on Honeycomb Tiles"
date: 2026-09-01 00:00:00 -0400  
categories: blog  
tags: [puzzles, probability]
excerpt: Solving Jane Street's Andy's Afternoon Amble puzzle.
image: /images/puzzles-andy.png
---

* TOC
{:toc}

This post will take you through my solution of Jane Street's August 2026 Puzzle, [Andy's Afternoon Amble](https://www.janestreet.com/puzzles/andys-afternoon-amble-index/). As always, I recommend you try it out yourself before reading this article!

The geometry here was big fun. Thinking about tetrahedrons took me back to organic chemistry class. You can go overboard with geometric thinking here, but the solution ends up being satisfyingly simple.

## Andy's motion on his sphere

What does it mean for Andy to "discover that he is no longer on the truncated 
tetrahedral sphere?"

Well, he marks his home by smell, remembers his turns, and knows the shape of his home, so he will figure it out if he makes a sequence of moves that would get him home on the truncated tetrahedral sphere but does not get him home on the floor.

So, what sequences of moves would get him home on the sphere? 

Let's think about each of his moves on the sphere as a "rotation" around one of the three black triangles bordering his current white hexagon, where he moves to one of the two white hexagons that also border that black triangle. 

{% include figures/andy-2026-09/style.html %}

{% include figures/andy-2026-09/solid-home.html %}

Clearly, 3 turns in single direction around one of the 3 black triangles that border his home will get him home. Let's call that a 3x turn.

Also, since the sphere has 1 black triangle that does not border his home hexagon, he can do an infinite number of rotations around that black triangle without getting home.

## Andy's motion on the floor

Now, let's think about what happens what will happen when Andy falls onto the floor. First, he marks his initial tile as home. Let's mark that tile blue.

{% include figures/andy-2026-09/floor-home.html %}

When Andy walks on the floor, we can still think of his moves as rotations, but now they are rotations around the black hexagons. 

If Andy does a 3x turn, he will realize that he should be home already. Let's call those tiles that should be home but are not home "fake homes", and let's mark these initial fake homes red.

{% include figures/andy-2026-09/floor-fakes-first.html %}

 Andy should be able to get back to his home tile by any 3x turn from his home. So, any other tile that can be reached by a 3x turn from a fake home will also be a fake home. Let's mark those red as well. If Andy hits any of these fake homes, he will realize that he is not on his sphere.

{% include figures/andy-2026-09/floor-fakes-all.html %}

Here's another way to think about which tiles will be recognized as fake homes. Remember the 1 triangle that does not border Andy's home on his sphere. That triangle borders each of the hexagons that are not Andy's home, so any tile that does not border that triangle will be Andy's home! See this model, which has home in blue and the non-bordering triangle marked yellow.

{% include figures/andy-2026-09/solid-yellow.html %}

Translate that logic to Andy's path on the floor. After Andy's first move in any direction, he will see what he thinks is the triangle that does not border his home. To him, any white hexagon that does not border that triangle should be his home. In other words, once he's orbiting a non-bordering triangle, any other non-orbit move should take him home. Let's mark those hexagons that he thinks are his non-bordering trangles yellow.

{% include figures/andy-2026-09/floor-yellow.html %}

Now, let's think about the paths he can take. Andy's first step's direction does not matter, because the lattice has rotational symmetry around the home tile. So just assume that he stepped in some direction and call it tile 1.

{% include figures/andy-2026-09/floor-tile1.html %}

Andy's next step has three options: back onto home, or onto one of the other two tiles adjacent to tile 1.

The lattice has reflective symmetry across the home - tile 1 axis, so paths starting from either of those two other tiles adjacent to tile 1 are equally likely to return home before they hit a "fake home". Let's call those tile 2.

{% include figures/andy-2026-09/floor-tile2.html %}

And so on, numbering the two tile 3s and the single tile 4 opposite our tile 1.

{% include figures/andy-2026-09/floor-tiles.html %}

## Visualizing moves on the sphere and corresponding moves on the floor

{% include figures/andy-2026-09/linked-walk.html %}

## The random walk

Let's call the probability that Andy gets back to his home before he hits a fake home starting from tile $$n$$ $$p_n$$

Andy is equally likely to move to any of his current adjacent tiles. So, starting at tile 1, he has a (1/3) chance to go home immediately and a (2/3) chance to proceed to tile 2. Andy can still get home from tile 2 with probability $$p_2$$, so the probability that he goes to tile 2 and still gets home is the product of those two probabilities, $$\frac{2}{3}p_2$$. 

We can therefore write the probability that he gets home from tile 1 as:

$$p_1 = \frac{1}{3} + \frac{2}{3}p_2 \tag{1}$$

Or, stated generally, because the probability of getting home from home is 1,

$$p_n = \sum_{m \in \{\text{ neighboring white tiles of } n\}}{\Pr(\text{Andy moves to tile } m \text{ from tile } n)\, p_m}$$

Now, from tile 2, Andy is equally likely to hit a fake home, return to tile 1, or proceed to tile 3. So, we can write the probability that he gets home from tile 2 as:

$$p_2 = \frac{1}{3}p_1 + \frac{1}{3}p_3 \tag{2}$$

And by similar logic:

$$p_3 = \frac{1}{3}p_2 + \frac{1}{3}p_4 \tag{3}$$

$$p_4 = \frac{2}{3}p_3 \tag{4}$$

This is a simple system of equations. Solve:

$$p_4 = \frac{2}{3}p_3$$

$$p_3 = \frac{3}{7}p_2$$

$$p_2 = \frac{7}{18}p_1$$

And we see that $$p_1 = \frac{9}{20}$$.

The questions asks for the probability that Andy has discovered that he is not on his sphere. Clearly, he will discover that if he hits a fake home before he gets back to his real home, so the answer is:

$$p = 1 - p_1 = \frac{11}{20}.$$
