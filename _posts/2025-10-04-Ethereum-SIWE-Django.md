---
layout: post  
title: "Hybrid dApps Part 1: Implementing Sign-In with Ethereum in Django"
date: 2025-10-04 17:00:00 -0400  
categories: blog  
tags: [web development, ethereum, evm, siwe, django]
---


* TOC
{:toc}

## What is SIWE?

[Sign-in With Ethereum](https://docs.login.xyz/) (SIWE) is a method for using Ethereum addresses to authenticate on off-chain services. Implementing SIWE means that your users don't need to create a separate account on your website as long as they already have an address on Ethereum or any other EVM-compatible chain. 

SIWE allows authentication on decentralized apps (dApps), where users must already have an address to transact. Authenticating with that address saves them from needing to manage separate login info.

### The SIWE Method

SIWE relies on the same public-key cryptography that addresses use to transact on EVM chains. In brief:

1. The server sends a cryptographic nonce to the client. This nonce is just a one-time-use randomly generated string. This nonce will be included in the message in the next step to prevent replay attacks, where an attacker would record a user's signed SIWE message and attempt to authenticate as that user by sending your server the same signed message.
2. The server sends a message to the client for signing. That message attests at least these fields:
    - The user's address
    - The domain requesting the SIWE
    - SIWE message version
    - The EVM chain id that your site uses
    - The URI of the resource that will validate the SIWE message and authenticate the user
    - The nonce from step 1
    - The time that the message was issued

3. The user signs the SIWE message with their private key and sends it to the site's login endpoint along with their address.
4. The server checks the signature to make sure that the true owner of the address sent the signed SIWE message. The server also verifies the message's fields.

### An Example SIWE Message

```
your-dapp.xyz wants you to sign in with your Ethereum Address:
0x105d00D5f671B236AfED1D2EEF5D7d566382E8C3

URI: your-dapp.xyz/login
Message Version: 2.1
Chain ID: 1
Nonce: 92649128
Issued At: 2025-10-04T17:00:00Z
```

## Why would you use SIWE with Django?

Django is a commonly used Python web framework. It provides abstractions that allow you to easily create database models, set up CRUD and more complicated endpoints, and handle user permissions. 

OK, so Django is a conventional web framework. But SIWE is mostly for dApps, right? Why use it with Django? Well, dApps store data on blockchains, but you may want to store some user or app data off-chain to:

1) Save gas on storage space
2) Store data types like videos that have poor support on blockchains
3) Reduce latency

If you want to create a hybrid dApp that also stores data on a centralized server, Django is a good choice because of its rapid development, ease of maintenance, and good scalability. Here are some specific hybrid dApp use cases:
- You want to allow users to customize their dApp UI and save those customizations between sessions without storing that info on-chain
- Your dApp stores compressed attestations of data on the chain to save gas, so you need a conventional backend to store full-size records
- You want users to view their transaction history without waiting for a long and complicated blockchain query

So, to create a hybrid Django dApp, you will likely want to implement SIWE in Django.

## Implementing SIWE in Django

Your SIWE implementation will need three key components:

1) A SiweUser model that extends the base Django user model with an Ethereum address field
2) SIWE verification logic and an authentication backend
3) A SIWE login view

Let's put these components into their own django app within your project. 

```bash
python manage.py startapp siweauth
```

I did this for a project earlier this year, so you can also check out my implementation [here](https://github.com/matthew-ritch/facthound/tree/main/siweauth). Note that I used JWTs instead of Django's built-in session framework, so there will be some differences in how you manage user sessions.

### The SiweUser Model

Django's base user model has almost everything you need for basic auth and user management. We just need to add a field for the user's Ethereum address and a manager method to create users with just an address.

We will also need a model for the SIWE message's nonce. This nonce should be single-use and have an expiration time.

```python
# siweauth/models.py
"""
Database models for SIWE
"""

from django.db import models
from django.core.validators import RegexValidator
from django.core.exceptions import ValidationError
from django.contrib.auth.models import BaseUserManager, AbstractBaseUser
from web3 import Web3


def validate_ethereum_address(value):
    """
    Validate that an Ethereum address is correctly checksummed.
    
    Args:
        value: The Ethereum address to validate
        
    Raises:
        ValidationError: If the address is not a valid checksummed Ethereum address
    """
    if not Web3.isChecksumAddress(value):
        raise ValidationError


class Nonce(models.Model):
    """
    A temporary nonce used for SIWE authentication.
    
    Nonces are single-use random values that prevent replay attacks during
    authentication and have expiration times.
    """
    value = models.CharField(max_length=24, primary_key=True)
    expiration = models.DateTimeField()

    def __str__(self):
        return self.value


class SiweUserManager(BaseUserManager):
    """
    Manager for the custom SiweUser model.
    
    This manager provides methods for creating users authenticated via SIWE
    """
    def create_user_address(self, address):
        """
        Create and save a SiweUser with the given Ethereum address.
        
        Args:   
            address: The Ethereum wallet address
            
        Returns:
            SiweUser: The created user instance
            
        Raises:
            ValueError: If no address is provided
        """
        if not address:
            raise ValueError("SiweUsers must have an eth address")

        user = self.model(
            wallet=address,
        )

        user.save(using=self._db)
        return user


class SiweUser(AbstractBaseUser):
    """
    This model extends AbstractBaseUser to support authentication via SIWE.
    """
    wallet = models.CharField(
        verbose_name="Wallet Address",
        max_length=42,
        unique=True,
        primary_key=True,
        validators=[
            RegexValidator(regex=r"^0x[a-fA-F0-9]{40}$"),
            validate_ethereum_address,
        ],
    )
    username = models.CharField(max_length=150, blank=True, null=True, unique=True)
    email = models.CharField(max_length=150, blank=True, null=True, unique=True)
    is_admin = models.BooleanField(default=False)
    is_staff = models.BooleanField(default=False)
    password = models.CharField(max_length=128, blank=True)
    objects = SiweUserManager()

    USERNAME_FIELD = "wallet"
    REQUIRED_FIELDS = []

    def __str__(self):
        return self.wallet

    @property
    def is_superuser(self):
        return self.is_admin

    def has_perm(self, perm, obj=None):
        return self.is_admin

    def has_module_perms(self, app_label):
        return self.is_admin
```

Now, register the custom user model in your settings.py file.

```python
# your_project/settings.py
...
AUTH_USER_MODEL = "siweauth.SiweUser"
...
```

### SIWE Verification and Authentication

We will write custom authentication methods to verify SIWE methods and allow authentication by Ethereum address.

#### SIWE Verification

First, let's create some settings for those SIWE message fields. Our validation will check that the messages' fields match these expected values. These settings are hardcoded so you can see them, but you should set yours in environment variables for security.

```python
# your_project/settings.py
...
# SIWE message validity window in minutes
SIWE_MESSAGE_VALIDITY = 5  # 5 minutes
# Expected chain ID for SIWE messages
SIWE_CHAIN_ID = 11155111 # Ethereum sepolia testnet
# Expected domain for SIWE messages
SIWE_DOMAIN = "your-dapp.xyz"
# Expected URI for SIWE messages
SIWE_URI = "https://your-dapp.xyz/login"
...
```

Now, we need to write some functions to verify SIWE messages using those settings. 
- `_nonce_is_valid` ensures that our SIWE messages' nonces are valid.
- `parse_siwe_message` splits our plaintext SIWE messages into fields for validation.
- `check_siwe` validates those fields.

```python
# siweauth/auth.py
import datetime, pytz
from web3 import Web3
from hexbytes import HexBytes
from eth_account.messages import encode_defunct
import re
from your_project.settings import (
    SIWE_MESSAGE_VALIDITY,
    SIWE_CHAIN_ID,
    SIWE_DOMAIN,
    SIWE_URI,
)
from siweauth.models import Nonce

w3 = Web3()


def _nonce_is_valid(nonce: str) -> bool:
    """
    Check if given nonce exists and has not yet expired.
    :param nonce: The nonce string to validate.
    :return: True if valid else False.
    """
    n = Nonce.objects.filter(value=nonce).first()
    is_valid = False
    if n is not None:
        if n.expiration > datetime.now(tz=pytz.UTC):
            is_valid = True
            n.delete()
    return is_valid


def parse_siwe_message(message_body: str) -> dict:
    """
    Parse SIWE message into components.
    Returns a dict or None if parsing fails.
    """
    try:
        domain_match = re.match(r"^([^\s]+) wants you to sign in with your Ethereum Address:", message_body)
        address_match = re.search(r"Ethereum Address:\n(0x[a-fA-F0-9]{40})", message_body)
        uri_match = re.search(r"^URI:\s*(.+)$", message_body, re.MULTILINE)
        version_match = re.search(r"^Message Version:\s*(.+)$", message_body, re.MULTILINE)
        chain_id_match = re.search(r"^Chain ID:\s*(\d+)$", message_body, re.MULTILINE)
        nonce_match = re.search(r"^Nonce:\s*(.+)$", message_body, re.MULTILINE)
        issued_at_match = re.search(r"^Issued At:\s*(.+)$", message_body, re.MULTILINE)

        if not (domain_match and address_match and uri_match and version_match and chain_id_match and nonce_match and issued_at_match):
            return None

        domain = domain_match.group(1).strip()
        address = address_match.group(1).strip()
        uri = uri_match.group(1).strip()
        version = version_match.group(1).strip()
        chain_id = int(chain_id_match.group(1))
        nonce = nonce_match.group(1).strip()
        issued_at = datetime.fromisoformat(issued_at_match.group(1).strip())

        return {
            "domain": domain,
            "address": address,
            "uri": uri,
            "version": version,
            "chain_id": chain_id,
            "nonce": nonce,
            "issued_at": issued_at,
        }
    except Exception as e:
        # Optionally log the error here
        return None


def check_siwe(message, signed_message):
    # check for format
    if type(message) != str:
        body = str(message.decode())
    else:
        body = message
    # Parse message components
    parsed = parse_siwe_message(body)
    if not parsed:
        return None
    # Validate timestamp
    now = datetime.now(parsed["issued_at"].tzinfo)
    if abs((now - parsed["issued_at"]).total_seconds()) > SIWE_MESSAGE_VALIDITY * 60:
        return None

    # Validate chain ID
    if parsed["chain_id"] != SIWE_CHAIN_ID:
        return None

    # Validate domain and URI
    if parsed["domain"] != SIWE_DOMAIN or parsed["uri"] != SIWE_URI:
        return None

    # check for nonce in db
    if not _nonce_is_valid(parsed["nonce"]):
        return None
    # recover address from nonce / signed message
    address = parsed["address"]
    try:
        recovered_address = w3.eth.account.recover_message(
            signable_message=encode_defunct(text=body),
            signature=HexBytes(signed_message),
        )
    except:
        return None
    # make sure recovered address is correct
    if address != recovered_address:
        return None
    return recovered_address
```

#### SIWE Authentication

A Django authentication backend is a class that implements two methods, `authenticate` and `get_user`. Let's write a custom authentication backend that uses our verification methods to implement those methods with SIWE.

```python
# siweauth/backend.py

from django.contrib.auth.backends import BaseBackend
from web3 import Web3
from eth_account.messages import SignableMessage
import logging

from siweauth.models import SiweUser
from siweauth.auth import check_siwe

w3 = Web3()

class SiweBackend(BaseBackend):
    """
    Authentication backend for Sign-In with Ethereum.
    """

    def authenticate(
        self, request, message: SignableMessage = None, signed_message=None
    ):
        """
        Authenticate a user with SIWE
        
        Args:
            request: The HTTP request
            message: The SIWE message
            signed_message: The signature of the message
            
        Returns:
            SiweUser: The authenticated user, or None if authentication fails
            
        Note:
            Creates a new user if one doesn't exist for the recovered address
        """
        # request must have message and signed_message fields
        if None in [message, signed_message]:
            return None
        recovered_address = check_siwe(message, signed_message)
        if recovered_address is None:
            return None
        # if user exists, return user
        user = SiweUser.objects.filter(wallet=recovered_address).first()
        # if user doesn't exist, make a user for this wallet
        if user is None:
            user = SiweUser.objects.create_user_address(recovered_address)
        return user

    def get_user(self, user_address):
        """
        Retrieve a user by user address.
        
        Args:
            user_address: The address of the user to retrieve
            
        Returns:
            SiweUser: The user with the given address, or None if not found
        """
        try:
            return SiweUser.objects.get(pk=user_address)
        except SiweUser.DoesNotExist:
            return None
```

### SIWE Login View

Finally, we need a view that will handle SIWE login requests. This view will call our custom SIWE authentication backend and log the user in if they pass our SIWE verification. Here is a super simple view for this purpose:

```python
# siweauth/views.py
from django.contrib.auth import login
from your_siwe_package import authenticate

def siwe_login_view(request):
    message = request.POST.get("message")
    signed_message = request.POST.get("signature")
    # This calls your SIWE authentication backend
    user = authenticate(message=message, signed_message=signed_message)
    if user is not None:
        login(request, user)  # This creates a session and sets the sessionid cookie
        return JsonResponse({"success": True})
    else:
        return JsonResponse({"success": False}, status=401)
```

Here's an example request to this view:

```http
POST /login HTTP/1.1
Host: your-dapp.xyz
Content-Type: application/x-www-form-urlencoded
message=your_siwe_message&signature=0xYourSignature
```

where `your_siwe_message` is the [plaintext SIWE message](#example-siwe-message), and `0xYourSignature` is the signature of that message.

### Testing your implementation

Testing your SIWE implementation is essential for maintaining security. To get some ideas for tests, check out my implementation's [siweauth tests](https://github.com/matthew-ritch/facthound/tree/main/siweauth/tests.py). In general, you will need to

- Test your Nonce model functions
- Test your SIWE message parsing and verification methods
- Test your views for getting nonces and for submitting SIWE messages

## Conclusion

Rig up your frontend to use these login views and you will be off to the races with your hybrid dApp. Your users will be able to log in with Ethereum and use your app without breaking their decentralized flow, but you will still be able to track user info with a performant and scalable centralized database.
