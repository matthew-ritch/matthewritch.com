---
layout: default
title: Blog
---

<style>
ul.blog-list li {
    margin-bottom: 1em;
}
</style>

<ul class="blog-list">
    {% for post in site.posts %}
    <li>
        <a href="{{ post.url }}">{{ post.title }}</a> - {{ post.date | date: "%B %d, %Y" }}
    </li>
    {% endfor %}
</ul>
