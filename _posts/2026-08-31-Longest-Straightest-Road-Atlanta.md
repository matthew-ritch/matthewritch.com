---
layout: post
title: "Longest Straightest Road in Atlanta"
date: 2026-08-31 12:00:00 -0400
categories: blog
tags: [optimization]
excerpt: Finding the longest, straightest line in Atlanta from Census TIGER data. Fun with Pareto fronts.
image: /images/map.png
---

* TOC
{:toc}

## Dataset

- All roads in Georgia from TIGER 2022
- TIGER is the Census's road data
- I am only considering roads which fit inside the bounds of figures two and three

{% include figures/lsl-2026-08/style.html %}

{% include figures/lsl-2026-08/road-maps.html %}

## Calculating length of road

This is pretty straightforward.

{% include figures/lsl-2026-08/length-hist.html %}

## Calculating Curvature of Road

Call curvature $$\sigma$$.

$$\sigma = \frac{l_{road} - l_{EndpointLine}}{l_{EndpointLine}}$$

{% include figures/lsl-2026-08/curvature-example.html %}

{% include figures/lsl-2026-08/curvature-hist.html %}

## Now we're getting somewhere

{% include figures/lsl-2026-08/length-vs-curvature.html %}

## Pareto

We want the straightest, longest road. We must make a tradeoff between length and curvature. Any road which is both shorter and curvier than another road cannot be our straightest, longest road. The grouping of roads which are not both shorter and curvier than another road are our Pareto Front.

{% include figures/lsl-2026-08/pareto-front.html %}

## Longest Straightest Lines

Here are the Pareto Front roads in table form. Looks reasonable.

{% include figures/lsl-2026-08/pareto-table.html %}


{% include figures/lsl-2026-08/longest-straightest-map.html %}

## Shortest Curviest Lines

{% include figures/lsl-2026-08/antipareto-front.html %}

{% include figures/lsl-2026-08/antipareto-table.html %}

{% include figures/lsl-2026-08/shortest-curviest-map.html %}

## Explorer

Click a point, a row, or the map itself.

{% include figures/lsl-2026-08/curvature-explorer.html %}
