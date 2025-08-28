---
layout: post  
title: "Web Dev for Dirtbags Part 1: A simple static site"  
date: 2025-08-27 10:30:00 -0400  
categories: blog  
tags: [web development, nginx, digitalocean, static site, dirtbag]
---

I like simple web pages. The simplest web pages are raw, static HTML files served over the internet.  

Raw files are written directly in HTML and contain all the necessary information to display a web page. Static pages do not change based on user input and will always display the same content.  

Let me show you what I mean. Open a text editor and paste this text into a new file. Save it as `index.html`.  

```html
<!DOCTYPE html>
<html>
    <head>
        <meta charset="UTF-8">
        <title>Web Dev for Dirtbags Part 1</title>
    </head>
    <body>
        Hello, Dirtbag!
    </body>
</html>
```

Double click that file and your browser will display a simple page. You get the idea.  

There are [plenty of guides](https://www.google.com/search?q=writing+raw+html) out there to help you write and style nice static web pages using HTML and CSS.  

# A basic example  

For the rest of this guide, I will assume that you have a web page with three files: `index.html`, `page2.html`, and `style.css`.  

`index.html`:  
```html
<!DOCTYPE html>
<html>
    <head>
        <meta charset="UTF-8">
        <title>This is index.html</title>
        <link rel="stylesheet" href="style.css">
    </head>
    <body>
        <h1>Hello, Dirtbag!</h1>
        <p>This is a simple static web page.</p>
        <a href="page2.html">Go to Page 2</a>
    </body>
</html>
```

`page2.html`:  
```html
<!DOCTYPE html>
<html>
    <head>
        <meta charset="UTF-8">
        <title>This is page2.html</title>
        <link rel="stylesheet" href="style.css">
    </head>
    <body>
        <h1>Go home, Dirtbag!</h1>
        <p>This is another simple static web page.</p>
        <a href="index.html">Go back home</a>
    </body>
</html>
```

`style.css`:  
```css
body {
    font-family: Arial, sans-serif;
    margin: 0;
    padding: 0;
}
h1 {
    color: #4b2802ff;
}
p {
    color: #7b1babff;
}
```

# Serving  

Once you are happy with your page, you need to "serve" it so that others can access it as a web page. To do this, we will:  

1. Register a domain name on a domain name registrar like Namecheap.
2. Rent a virtual private server (VPS) from a service like DigitalOcean.
3. Deploy your files to the VPS.
4. Make an SSL certificate for your domain.
5. Configure a web server for static file hosting. We will use Nginx for this.
6. Point your domain name to the VPS.


## Register a domain name  

Domain names are just human-readable addresses for your website. When you open a web browser and type in a URL, the browser uses the domain name to locate the server hosting the website and ask it for the page you want to see. Take `https://www.thecleaners.ai` for instance. The domain name is `thecleaners.ai`, and it points to a specific server on the internet.  
  
Domain names must be registered with a domain name registrar. There are many registrars to choose from, such as Namecheap, GoDaddy, and Google Domains. Simply create an account with one of these registrars, search for a domain name that suits your page, and follow the instructions to register it. You can probably find a suitable one for less than $10.   


## Rent a VPS  

To serve your website, you need a computer that will sit there waiting for requests from users. You could use your laptop for this, but that means that you would need to keep it on with a reliable internet connection 24/7 and expose its contents to the internet.   
  
Instead rent a virtual private server (VPS) from a service like DigitalOcean. A VPS can easily sit there for you 24/7 and answer requests and won't have any of your personal files on it, so this is a much better option.  
  
While setting up your VPS, they will present you with options for your server's resources like CPU, RAM, and storage. Your site is simple, so pick the cheapest option. Namecheap has a $4/month budget option.  
  
Once you have set this up, note the IP address. Try to [`ssh`](https://www.man7.org/linux/man-pages/man1/ssh.1.html) (Secure Shell) into your VPS using the following command:  
  
```bash
ssh username@vps_ip_address
```
  
Replace `username` with your VPS username (configure this on the provider's dashboard) and `vps_ip_address` with the IP address of your VPS (find this in the provider's dashboard). You could also set up SSH keys for passwordless login, but don't worry about that if you don't know what that means.  
  
If your ssh worked, you should see a terminal prompt for your VPS. You can now run commands on your VPS as if you were sitting in front of it.  
  
While you're connected, go ahead and create a directory for your website files:  

```bash
mkdir -p /var/www/your_website
```


## Deploy your files  

Next, to serve your website, you need to get your files to the VPS. The most common way is to use `git` and a remote repository on a service like GitHub (free for you, most likely). This allows you to easily update your code in the future.  

However, if you don't know what `git` is or if you're a real dirtbag, you can use [`scp`](https://man7.org/linux/man-pages/man1/scp.1.html) (secure copy) to copy files between your local machine and the VPS.  

To use `scp`, open a terminal (or PowerShell on Windows) on your local machine and run the following commands:  

```bash
zip -r site.zip /path/to/your/folderwithallfiles # create a zip archive of your site
scp site.zip username@vps_ip_address:/var/www/your_website # copy the zip file to the VPS
```

Then, ssh into the VPS:  

```bash
ssh username@vps_ip_address # ssh into the VPS
ls /var/www/your_website
unzip site.zip # extract the zip file on the VPS
```

Replace `/path/to/your/folderwithallfiles` with the path to the folder with all of your site files, `username` with your VPS username, `vps_ip_address` with the IP address of your VPS, and `/var/www/your_website` with the VPS directory you created to hold the site files.  


## Set up SSL  

People won't trust your website if you are not using HTTPS. To serve your website over HTTPS, you need an SSL certificate.  

The easiest way to get one is with [Let's Encrypt](https://letsencrypt.org/), which provides free SSL certificates.  

1. Install Certbot on your VPS:  
```bash
sudo apt install certbot python3-certbot-nginx
```
2. Obtain an SSL certificate:  
```bash
sudo certbot --nginx -d your_domain.com -d www.your_domain.com
```

Follow the prompts to complete the certificate issuance process.  

Note where the SSL certificate files are stored (usually in `/etc/letsencrypt/live/your_domain.com/`).  


## Configure Nginx for static file hosting  

Now that your VPS has your website files, you need to tell it how to serve them to site visitors. We'll use Nginx for this.  

1. Install Nginx on your VPS:  
```bash
sudo apt update
sudo apt install nginx
```
2. Create a new Nginx configuration file for your website:  
```bash
sudo nano /etc/nginx/sites-available/your_website
```

3. Add the following configuration to the file:  
```nginx
server {
    listen 80;
    server_name your_domain.com www.your_domain.com;
    return 301 https://$host$request_uri;
}
server {
    listen 443 ssl;
    server_name your_domain.com www.your_domain.com;

    root /var/www/your_website;
    index index.html;

    ssl_certificate /etc/letsencrypt/live/your_domain.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/your_domain.com/privkey.pem;

    location / {
        try_files $uri $uri/ =404;
    }
}
```
Replace `your_domain.com` with your actual domain and set the correct paths to your SSL certificate files.  
4. Enable the new site configuration:  
```bash
sudo ln -s /etc/nginx/sites-available/your_website /etc/nginx/sites-enabled/
```
5. Test the Nginx configuration for syntax errors:  
```bash
sudo nginx -t
```
6. If the test is successful, restart Nginx:  
```bash
sudo systemctl restart nginx
```


## Point your domain name to the VPS  

Finally, you need to point your domain name to your VPS's IP address.  

First, on your domain name registrar's website, Follow your registrar's instructions for delegating nameservers to your VPS.   

See your registrar's documentation for specific instructions about nameserver. If DigitalOcean is your VPS provider, follow [this guide](https://docs.digitalocean.com/products/networking/dns/getting-started/dns-registrars/) and add the following nameservers: `ns1.digitalocean.com`, `ns2.digitalocean.com`, and `ns3.digitalocean.com`.  

Second, on your VPS provider's control panel, update the DNS settings for your domain to point to your VPS's IP address. Your VPS provider should have documentation on how to do this. For DigitalOcean, see [this guide](https://docs.digitalocean.com/products/networking/dns/getting-started/quickstart/).  

Now, after a few minutes, your domain should point to your VPS, and you should be able to access your website using your domain name!
