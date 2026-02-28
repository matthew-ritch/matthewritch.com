---
layout: post  
title: "Web Dev for Dirtbags Part 3: VPS Security"
date: 2025-09-03 08:00:00 -0400  
categories: blog  
tags: [web development, nginx, digitalocean, ssh, security, dirtbag]
excerpt: Learn some easy steps to harden your web server's security.
---

<!-- - vps security basics
        - ssh access only https://docs.digitalocean.com/products/droplets/how-to/add-ssh-keys/to-existing-droplet/#with-ssh-copy-id
        - ufw setup 
        - unattended upgrades?
        - systemd service hardening?
        - fail2ban 
        - nginx https://github.com/trimstray/nginx-admins-handbook/blob/master/doc/RULES.md#beginner-always-keep-nginx-up-to-date
-->

* TOC
{:toc}

## Purpose: security

You should take a few simple steps to harden your web server's security. In this guide, I will show you how to:
1. Lock down access with ssh keys, a firewall, and fail2ban for banning vulnerability scanners
2. Enable unattended upgrades to prevent fresh exploits
3. Restrict permissions for systemd services and Nginx

These instructions are for Ubuntu/Debian-based systems. This probably applies to your cheap VPS, but you should make sure.

## Prerequisites

I will also assume that you have a web server set up and ready to serve a website. If you don't have these set up yet, I recommend reading and following my previous posts on serving [static](https://matthewritch.com{% link _posts/2025-08-27-WDD-Static.md %}) and [dynamic](https://matthewritch.com{% link _posts/2025-09-02-WDD-Django-React.md %}) sites.

## Lock down access

Your server and your users' data can be accessed directly through ssh or indirectly through the pages that you are serving over its ports. Let's close some of those openings.

### ssh keys

To start, let's make sure that your server can only be accessed through ssh keys. ssh keys are more secure than username and password logins because they are much harder to brute force or guess. Additionally, they are not transmitted across the network to your remote VPS, so your password cannot be intercepted during transmission.

To set up ssh key access to your remote machine:

1. On your local machine, run `ssh-keygen -t ed25519` to generate a new key pair with the ed25519 algorithm. Set up a passphrase when prompted for extra security. 
2. Copy the public key that you just generated, by default `id_ed25519.pub`, over to the `~/.ssh/authorized_keys` folder on your VPS:
```bash
cat ~/.ssh/id_ed25519.pub | ssh username@your-vps-ip "mkdir -p ~/.ssh && cat >> ~/.ssh/authorized_keys"
```
Replace `username` with your VPS username and `your-vps-ip` with your server's IP address.
If you don't have ssh access, create this file and add the public key manually via your VPS provider's control panel.
3. Add the private key to your ssh agent:
```bash
eval "$(ssh-agent -s)" # for linux and mac. for windows use ssh-agent -s
ssh-add ~/.ssh/id_ed25519 # or whatever file name you gave your private key
```
4. Test your ssh key authentication by running:
```bash
ssh username@your-vps-ip
```
If you were able to log in without your password, then this worked!

ONLY ONCE YOU CONFIRM THAT YOU CAN SSH IN WITH THE KEY, you can disable password authentication:

1. On your remote machine, modify `/etc/ssh/sshd_config` to include this line:
```bash
PasswordAuthentication no
```
2. Restart the SSH service:
```bash
sudo systemctl restart ssh
```
If everything is set up correctly, you should be able to log in without being prompted for a password.

### Firewall

A firewall is a service that filters and blocks connections based on predefined rules. [Uncomplicated Firewall](https://wiki.ubuntu.com/UncomplicatedFirewall), or `ufw`, is a simple and user-friendly interface for defining a firewall. 

To set up `ufw` on your VPS:

1. Make sure you have ssh access to your server. Follow the previous section if you don't. Log in using your SSH key.
2. Install `ufw` if it's not already installed:
```bash
sudo apt-get install ufw
```
3. Allow SSH connections and HTTP/HTTPS traffic:
```bash
sudo ufw allow ssh
sudo ufw allow http
sudo ufw allow https
```
4. Enable the firewall:
```bash
sudo ufw enable
```
5. Check the status of the firewall:
```bash
sudo ufw status
```


### Blacklisting and Fail2ban

Automated vulnerability-finders are constantly scanning open ports for accidentally exposed files. If you check the access logs for your site, you will probably see loads of failed requests looking for seemingly random .php, .xml, .env, or other sensitive files. Here's an example log trace from my website.
```console
20.196.80.178 - - [03/Sep/2025:16:14:41 +0000] "GET /dfre.php HTTP/1.1" 404 162 "-" "-"
20.196.80.178 - - [03/Sep/2025:16:14:42 +0000] "GET /disagimons.php HTTP/1.1" 404 162 "-" "-"
20.196.80.178 - - [03/Sep/2025:16:14:42 +0000] "GET /disagreop.php HTTP/1.1" 404 162 "-" "-"
20.196.80.178 - - [03/Sep/2025:16:14:43 +0000] "GET /manager.php HTTP/1.1" 404 162 "-" "-"
20.196.80.178 - - [03/Sep/2025:16:14:43 +0000] "GET /uploan.php HTTP/1.1" 404 162 "-" "-"
104.248.157.27 - - [03/Sep/2025:16:35:29 +0000] "GET //wp-includes/wlwmanifest.xml HTTP/1.1" 404 564 "-" "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36"
104.248.157.27 - - [03/Sep/2025:16:35:29 +0000] "GET //xmlrpc.php?rsd HTTP/1.1" 404 564 "-" "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36"
104.248.157.27 - - [03/Sep/2025:16:35:29 +0000] "GET //blog/wp-includes/wlwmanifest.xml HTTP/1.1" 404 564 "-" "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/78.0.3904.108 Safari/537.36"
```

This shows automated scanning from two separate source IP addresses. Notice that each of these requests are returned 404 not found errors. 

[Fail2ban](https://github.com/fail2ban/fail2ban) is a service that monitors requests to your page for suspicious activity. It can automatically block IP addresses that show malicious signs, such as too many password failures or seeking for exploits like those shown in the log.

To set up Fail2ban on your VPS:

1. Install Fail2ban:
```bash
sudo apt-get install fail2ban
```
2. Configure Fail2ban to block repeated failed requestors:
```bash
sudo cp /etc/fail2ban/jail.conf /etc/fail2ban/jail.local
sudo nano /etc/fail2ban/jail.local
```
Within the `[DEFAULT]` section, set these values:
```
bantime = 1h
findtime = 1h
maxretry = 5
```
3. Start the Fail2ban service:
```bash
sudo systemctl start fail2ban
```
4. Enable Fail2ban to start on boot:
```bash
sudo systemctl enable fail2ban
```



## Keep your installs fresh

Another key aspect of server security is to stay on top of updates for your dependencies and server software.

### System packages

To update your system packages, use the following commands:

```bash
sudo apt-get update
sudo apt-get upgrade
```

To set up automatic updates, you can install and enable the `unattended-upgrades` package, which automatically installs security updates:

```bash
sudo apt-get install unattended-upgrades
sudo dpkg-reconfigure --priority=low unattended-upgrades
```

### npm

For Node.js applications, you can update your npm packages with:

```bash
npm update
```

There are packages for conducting automatic updates for npm, but it's simpler to set a cron job to execute `npm update` at your desired frequency.
```bash
nano /etc/cron.daily/npm-update.sh
```
Copy this into `nano /etc/cron.daily/npm-update.sh`:
```bash
#!/bin/bash
cd /path/to/your/nodejs/app
npm update
```
And give the cron job execute permissions:
```bash
chmod +x /etc/cron.daily/npm-update.sh
```

### Python venv

For Python applications, you can update your virtual environment packages with:

```bash
source /path/to/your/venv/bin/activate
pip install --upgrade pip
pip list --outdated | awk 'NR>2 {print $1}' | xargs -n1 pip install -U
deactivate
```

You can also set a cron job to automate this process.
```bash
nano /etc/cron.daily/venv-update.sh
```
Copy this into `nano /etc/cron.daily/venv-update.sh`:
```bash
#!/bin/bash
source /path/to/your/venv/bin/activate
pip install --upgrade pip
pip list --outdated | awk 'NR>2 {print $1}' | xargs -n1 pip install -U
deactivate
```
And give the cron job execute permissions:
```bash
chmod +x /etc/cron.daily/venv-update.sh
```

## Restrict custom systemd services and Nginx

Your [systemd](https://www.man7.org/linux/man-pages/man1/init.1.html) services and your [Nginx](https://nginx.org/en/docs/) configuration should be set up to use the minimal permissions necessary to function. This means:
1. Running services with unprivileged users and groups
2. Add systemd security options to each service
3. Configure nginx to also use the same restricted user and group as your systemd services


You can run `systemd-analyze security` on your server to see a list of services and their vulnerability levels. 
To address these vulnerabilities, you must restrict each service in this list. 

Run `systemd-analyze security servicename` to see detailed information for each service.

### Restricting systemd services' permissions

1. Edit your systemd service file (something like `/etc/systemd/system/yoursite.service`) to contain user and group definitions for some unprivileged group. We will use `www-data` for both.
```bash
# filepath: /etc/systemd/system/yoursite.service
# ...existing code...
[Service]
User=www-data
Group=www-data
# ...existing code...
```
2. Restrict file permissions for the files the service accesses. For example, if your service accesses files in `/var/www/yoursite/servicefolder`, you can run:
```bash
sudo chown -R www-data:www-data /var/www/yoursite/servicefolder # change ownership to our user and group
sudo chmod -R 755 /var/www/yoursite/servicefolder # grants owner rwx, group rx, others rx
```
3. Restart the service to apply the changes:
```bash
sudo systemctl daemon-reload
sudo systemctl restart yoursite
```

### Add systemd security options

Add some extra protection to your service by restricting its capabilities. Do this for each service you use.

1. Edit your systemd service file (something like `/etc/systemd/system/yoursite.service`) to enable security options under the `[SERVICE]` header. For example:
```bash
# filename: /etc/systemd/system/yoursite.service
[Service]
PrivateNetwork=yes # make this service only accessible to internal network requests. Only use if Nginx is also configured to use this socket
PrivateTmp=yes # create a private /tmp for this service
ProtectHome=yes # prevent access to /home, /root, and /run/user 
NoNewPrivileges=true # prevent privilege escalation
ProtectSystem=full # make /usr read-only
ProtectKernelModules=yes # prevent loading kernel modules
```
2. Restart the service to apply the changes:
```bash
sudo systemctl daemon-reload
sudo systemctl restart yoursite
```

### Configure Nginx for security

Nginx starts as root so that it can access all of your server's files, such as SSL certs. However, you should ensure that it is configured to drop to a user with lower privileges after it starts up. We will do this with `user` directive in your Nginx configuration file, usually located at `/etc/nginx/nginx.conf`.

Make sure that the user is set to a lower privileged user, such as `www-data` from before:
```bash
# filename: /etc/nginx/nginx.conf
user www-data;
```

Nginx by default emits its version on error pages and in the “Server” response header field. To disable this, add the following line to your Nginx configuration file:
```bash
# filename: /etc/nginx/nginx.conf
server_tokens off;
```

Some HTTP methods may be used to exploit vulnerabilities. For instance, `PUT` can be used to replace your server files with malicious ones. We can restrict Nginx to only allow certain HTTP methods. For instance, if your app only needs to support `GET` and `POST`, you can restrict Nginx to those with this:
```bash
# filename: /etc/nginx/nginx.conf
server {
    location / {
        limit_except GET POST {
            deny all;
        }
    }
}
```

You can also add these restrictions to your site's specific Nginx configuration, locatated somewhere like `/etc/nginx/sites-available/your-site.conf`. The above modifications apply to all Nginx instances running on your server.
