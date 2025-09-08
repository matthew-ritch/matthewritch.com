---
layout: post  
title: "Web Dev for Dirtbags Part 2: Deploying a Django + React Webapp"
date: 2025-09-02 08:00:00 -0400  
categories: blog  
tags: [web development, nginx, digitalocean, gunicorn, django, react, dirtbag]
---

* TOC
{:toc}

## Purpose: hosting a dynamic app

We are going to get you off of localhost.

1. Learn to host a dynamic web app under your own domain name on your own server
2. Gain web deployment skills:
    - Configuring a web server (Nginx) for serving a front end and a back end on the same server
    - Managing application processes (e.g., using Gunicorn)

## Dynamic web pages

Dynamic web pages or apps are interactive and will change based on user input or database state.

The "front end" of a dynamic web app is the process responsible for generating and updating the user interface. It does this by communicating with the "back end" of the web app, which manages data storage and business logic while responding to the front end's requests. 

This guide will show you how to host a dynamic web page. We will use the tried and true Django back-end framework and the ubiquitous React front-end framework as concrete examples, but we will also discuss general principles that apply to the myriad other web dev frameworks.

## Prerequisites

For the rest of this guide, I will assume that you have basic [Django](https://docs.djangoproject.com/en/5.2/intro/tutorial01/) and [React](https://react.dev/learn) applications set up and running locally. Follow those links for the official tutorials if you need help getting started.

I will also assume that you have a Virtual Private Server (VPS) set up and ready to go and a domain name purchased and pointed at your VPS. If you don't have these set up yet, I recommend reading and following [my previous post](https://matthewritch.com{% link _posts/2025-08-27-WDD-Static.md %}) on setting up a VPS and domain name for hosting static sites.


## Configuring Django

1. Set up a Python virtual environment
2. Namespace all your back end's urls
3. Update your settings.py so your back end can serve requests from your front end
4. Configure Gunicorn, your Web Server Gateway Interface (WSGI)

### Python environment

Python comes stock with `venv`, a module for creating lightweight virtual environments. You are running this on a resource-constrained (cheap) VPS, so you should keep your venv lean.

From your dev environment, create a requirements.txt file by running `pip freeze > requirements.txt`. Copy this to your server and create a venv there with:
```bash
python3 -m venv yourvenv
source yourvenv/bin/activate
pip install -r requirements.txt
```

### Namespacing

You should namespace all of your back end endpoints with some common prefix so they don't clash with your front end's urls (e.g., `/api/v1/whatever`).

Later, this will allow us to route front-end requests and back-end requests to separate processes more easily. 

### `settings.py`

Because your back end and front end are separate processes and will have their own origins, you will need to configure a few things in your settings.py so your back end can serve requests from your front end. If you do not do these things, then your back end cannot answer requests from your front end.

Cross-site request forgery (CSRF) is an exploit where a site's browser is tricked into sending a request to a different site. This can cause the user to unwittingly submit forms on some other site (like their bank) or expose user information. To prevent this, browsers block cross-origin requests by default. However, because your front end and back end are on different origins, you will need to explicitly allow requests from your front end to your back end.

Cross-origin resource sharing (CORS) is a feature that permits browsers to make limited cross-origin requests. To enable your cross-origin requests, do the following:

1. In your server's python environment, install the `django-cors-headers` package:
```bash
pip install django-cors-headers
```

2. Add `corsheaders` to your `INSTALLED_APPS` in `settings.py`:
```python
INSTALLED_APPS = [
    # ...existing code...
    'corsheaders',
    # ...existing code...
]
```

3. Add the `CorsMiddleware` to your `MIDDLEWARE` in `settings.py`, making sure it's placed above `CommonMiddleware`:
```python
MIDDLEWARE = [
    # ...existing code...
    'corsheaders.middleware.CorsMiddleware',
    'django.middleware.common.CommonMiddleware',
    # ...existing code...
]
```

4. Specify the allowed origins in `settings.py`:
```python
CORS_ALLOWED_ORIGINS = [
    "https://your_domain.com",
    "http://localhost:3000", # for local development
]
```

5. Configure CSRF settings in `settings.py`:
```python
CSRF_TRUSTED_ORIGINS = [
    "https://your_domain.com",
    "http://localhost:3000", # for local development
]
```

To allow your server to be hosted on your domain, add the following to `settings.py`:

```python
ALLOWED_HOSTS = [
    "your_domain.com",
    "localhost", # for local development
]
```

Django's secret key is used for cryptographic signing. You should set it in a `.env` file and load it in `settings.py`. We will tell Gunicorn which `.env` file to use in the next section.



### Gunicorn

Gunicorn is a popular HTTP server for Python applications such as those built with Django or Flask. It receives HTTP requests from clients, forwards them to the Python process running your app, and forwards the Python app's response to the the client. It can respond to multiple requests concurrently.

Install it with `pip install gunicorn`.

Make sure that all project files are owned by the user that will run the Gunicorn process (usually `www-data` on Ubuntu). You can change the ownership with:

```bash
sudo chown -R www-data:www-data /var/www/yourproject
```

Test it with:

```bash
gunicorn your_project.wsgi:application
```

But, this is not how you should run Gunicorn on your server. Instead, you should use a service that will spin up Gunicorn when it is needed to serve a request and manage the process for you. `systemd` is the best choice for this.

To configure `systemd` to manage your Gunicorn process, we will create two files. `systemd.service` files define how the service will be controlled. `systemd.socket` files define how the service will be started.

1. Create a `systemd.socket` file for Gunicorn:
```ini
# filepath: /etc/systemd/system/yoursite.socket
[Unit]
Description=gunicorn socket
# Direct Gunicorn requests to this socket file
[Socket]
ListenStream=/run/yoursite.sock 
# Create this socket on boot
[Install]
WantedBy=sockets.target 
```
Make sure to replace `yoursite` with the actual name of your project.


2. Create a `systemd.service` file for Gunicorn:
```ini
# filepath: /etc/systemd/system/yoursite.service
[Unit]
Description=gunicorn daemon
# Require the file we created in step 1
Requires=yoursite.socket 
After=network.target
[Service]
# Allow this service to access back end files
User=www-data
Group=www-data
# Set base folder to the one that contains your Django project
WorkingDirectory=/var/www 
# 
ExecStart=/var/www/your-venv/bin/gunicorn \
          --access-logfile - \ # log to stdout
          --workers 3 \ # number of worker processes
          --bind unix:/run/yoursite.sock \ # This is the socket file from step 1 defined with ListenStream
		  --access-logfile /var/log/gunicorn/yoursite.access.log \
		  --error-logfile /var/log/gunicorn/yoursite.error.log \
          yoursite.wsgi:application
# If you have environment variables
EnvironmentFile=/var/www/yoursite/.env 
[Install]
WantedBy=multi-user.target
```
Make sure to replace `yoursite` with the actual name of your project, set your correct `WorkingDirectory` and `EnvironmentFile`, and pass the correct virtual environment path in `ExecStart`.

3. Start and test your new Gunicorn service:
```bash
sudo systemctl start yoursite.socket
sudo systemctl enable yoursite.socket
sudo systemctl status yoursite.socket
```
You should see something like this:
```console
● yoursite.socket - gunicorn socket
     Loaded: loaded (/etc/systemd/system/yoursite.socket; enabled; preset: enabled)
     Active: active (running) since Tue 2025-09-02 22:12:07 UTC; 7s ago
   Triggers: ● yoursite.service
     Listen: /run/yoursite.sock (Stream)
     CGroup: /system.slice/yoursite.socket
```

4. Make sure you created the socket file 
```bash
file /run/yoursite.sock
```
You should see something like this:
```console
/run/yoursite.sock: socket
```

5. If you see errors or if one of those prints looks wrong, try reading the logs
```bash
sudo journalctl -u yoursite.socket
```

6. To test the whole setup, try accessing an api endpoint with curl
```bash
curl --no-buffer -XGET --unix-socket /run/yoursite.sock localhost/api/endpoint/
```
and you should see the HTTP response from your endpoint.




## Configuring a React app for deployment

1. Install your dependencies and build your front-end app
2. Build your app


### `npm install`

Your resource-constrained VPS will have very little RAM, so you need to keep your environment lean. This is where the super-cheap VPS approach can easily fail. Luckily, we can use the `--production` flag to skip dev dependencies:

```bash
npm install --production
```

If you see weird errors or if this run takes a very long time, you are probably coming up against your VPS's RAM constraints.

We can get around this by using swap. Swap is disk memory that you can use to emulate RAM. Accessing swap is of course slower than accessing real RAM, but if you only need it for this one-time install, users will never see a slowdown from it.

Alternatively, install and build your app on your local machine and then transfer the build artifacts to your VPS.

### `npm run build`

Once your dependencies are installed, you can build your React app:

```bash
npm run build
```

This will create a `build` directory containing your compiled web app's files. If you are building on your local machine, just transfer the `build` directory to your VPS:
```bash
zip -r build.zip /path/to/your/build # create a zip archive of your site
scp build.zip username@vps_ip_address:/var/www/yoursite # copy the zip file to the VPS
```

## Configuring Nginx

Reverse proxies are intermediate servers that route client requests to the appropriate backend server. We will use Nginx as a reverse proxy to either forward user requests to the Gunicorn socket or serve the React app's static files.

1. If you don't already have it, install Nginx on your VPS:  
```bash
sudo apt update
sudo apt install nginx
```
2. Create a new Nginx configuration file for your website:  
```bash
sudo nano /etc/nginx/sites-available/yoursite
```

3. Add the following configuration to the file:  
```nginx
# Each server block defines a virtual server's routing behavior
# This server handles HTTP requests by redirecting them to HTTPS
server {
    # HTTP connections come in on port 80
    listen 80;
    server_name your_domain.com www.your_domain.com;
    # Redirect those HTTP requests to HTTPS
    return 301 https://$host$request_uri;
}
# This server handles HTTPS requests
server {
    # HTTPS connections come in on port 443
    listen 443 ssl;
    server_name your_domain.com www.your_domain.com;
    # SSL certificate files - tell Nginx where to find proof that the domain is yours
    ssl_certificate /etc/letsencrypt/live/your_domain.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/your_domain.com/privkey.pem;
	# Set logs
	access_log /var/log/nginx/yoursite.access.log;
	error_log /var/log/nginx/yoursite.error.log;
    # Root directory for the website: tells Nginx where to find the files to serve
    root /var/www/yoursite;
    # Default file to serve
    index index.html;
    # This location block forwards API requests to your Gunicorn process
    location /api/ {
        include proxy_params;
        proxy_pass         http://unix:/run/yoursite.sock;
    }
    # All other requests will be served with static files
    # This makes any file in /var/www/yoursite accessible, so make sure not to expose sensitive files
    # Any file that is not found will return a 404 error
    location / {
        try_files $uri $uri/ =404;
    }
}
```
Replace `your_domain.com` with your actual domain and set the correct paths to your SSL certificate files.  
**Make sure not to expose sensitive files in your web root directory!**
4. Enable the new site configuration:  
```bash
sudo ln -s /etc/nginx/sites-available/yoursite /etc/nginx/sites-enabled/
```
This creates a symbolic link between your site's configuration file in `sites-available` and the `sites-enabled` directory, which Nginx uses to determine which sites to serve.
5. Test the Nginx configuration for syntax errors and fix any errors reported in the output:
```bash
sudo nginx -t
```
6. If the test is successful, reload Nginx:  
```bash
sudo systemctl reload nginx
```

## Monitoring

We set up logging for both Nginx and Gunicorn to monitor the requests they are serving and any errors that pop up.

1. **Nginx Logs**: We configured access and error logs in the Nginx configuration file:
```nginx
	# ...
	# Set logs
	access_log /var/log/nginx/yoursite.access.log;
	error_log /var/log/nginx/yoursite.error.log;
	# ...
```

2. **Gunicorn Logs**: We can also configured Gunicorn to log to a file with the `--access-logfile` and `--error-logfile` options in the `systemd.service` file:
```bash
	# ...
	--access-logfile /var/log/gunicorn/yoursite.access.log
	--error-logfile /var/log/gunicorn/yoursite.error.log
	# ...
```

You can access these logs with either `cat` or `tail`. For instance:
```bash
# View the last 100 lines of the access log
tail -n 100 /var/log/nginx/yoursite.access.log
# View all of the Gunicorn error log
cat /var/log/gunicorn/yoursite.error.log
```
