---
layout: default
title: Blog
---

<link href="/styles/blogposts.css" rel="stylesheet">

<style>
.blog-list { list-style: none; padding: 0; margin: 0; }
.blog-list li { margin-bottom: 2em; }
.blog-item-title { font-size: 1.15em; font-weight: 700; }
.blog-item-meta { font-size: 0.85em; color: #6f6e69; margin-top: 0.15em; }
.blog-item-excerpt { margin-top: 0.4em; line-height: 1.5; }
.blog-item-tags { margin-top: 0.5em; }
.blog-item-tags a {
  display: inline-block; font-size: 0.75em; color: #52514e;
  background: rgba(137, 135, 129, 0.12); border-radius: 5px;
  padding: 0.15em 0.55em; margin-right: 0.35em; margin-bottom: 0.25em;
  text-decoration: none;
}
.blog-item-tags a:hover { color: #2a78d6; }
</style>

<h1>Blog</h1>

<ul class="blog-list">
    {% for post in site.posts %}
        <li>
            <a class="blog-item-title" href="{{ post.url | remove: '.html' }}">{{ post.title }}</a>
            <div class="blog-item-meta">{{ post.date | date: "%B %-d, %Y" }}</div>
            {% if post.excerpt %}
                <div class="blog-item-excerpt">{{ post.excerpt | strip_html | strip_newlines }}</div>
            {% endif %}
            {% if post.tags %}
                <div class="blog-item-tags">
                    {% for tag in post.tags %}
                        <a href="/tags/#{{ tag | slugify }}">{{ tag }}</a>
                    {% endfor %}
                </div>
            {% endif %}
        </li>
    {% endfor %}
</ul>
