#!/usr/bin/env bash
# Find plausible human traffic in nginx logs.
# Usage: humans.sh [logglob]   (default: ./logs/matthewritch.com.access.log*)
set -euo pipefail

GLOB=${1:-./logs/matthewritch.com.access.log*}
WORK=$(mktemp -d); trap 'rm -rf "$WORK"' EXIT

zcat -f $(ls -tr $GLOB) > "$WORK/all.log"

# ---------------------------------------------------------------- 1. UA denylist
# Self-identifying crawlers. This is the single biggest win and the log is honest
# about it: Googlebot, GPTBot, ClaudeBot, Bytespider, Ahrefs, meta-* all say so.
BOT_UA='bot|crawl|spider|slurp|scrape|fetch|monitor|check|scan|probe|search|index|
preview|render|archiv|feed|rss|http-?client|libwww|curl|wget|python|go-http|java/|
okhttp|axios|node-fetch|guzzle|headless|phantom|selenium|playwright|puppeteer|
facebookexternalhit|facebookcatalog|twitterbot|slackbot|discordbot|telegrambot|
whatsapp|linkedinbot|pinterest|redditbot|embedly|quora|applebot|petalbot|yandex|
baidu|sogou|bytedance|amazonbot|perplexity|anthropic|openai|cohere|diffbot|
dataprovider|semrush|ahrefs|majestic|moz\.com|dotbot|mj12|blexbot|serpstat|
seokicks|sitecheck|screamingfrog|netcraft|censys|shodan|zgrab|masscan|nmap|
expanse|internet-measurement|paloalto|rootevidence|networkingextension|
webindexer|externalagent|^-$|^$'

# ---------------------------------------------------------------- 2. exploit scanners
# Any IP that ever asks for wp-admin/.env/.php on a static Jekyll site is hostile.
# Ban the whole IP, not just the request.
awk '$7 ~ /(\.php|\.env|\.git|\.aws|wp-|xmlrpc|phpmyadmin|\/shell|\/config|\/vendor\/|\/cgi-bin|\/\.well-known\/.*\.php|autodiscover|\/admin)/ {print $1}' \
  "$WORK/all.log" | sort -u > "$WORK/scanners.txt"

# ---------------------------------------------------------------- 3. per-request scoring
# Keep only real page views from non-bot UAs on non-scanner IPs.
grep -viEf <(echo "$BOT_UA" | tr -d '\n' | tr '|' '\n' | sed '/^$/d') "$WORK/all.log" \
  | grep -vFf "$WORK/scanners.txt" > "$WORK/maybe.log" || true

# ---------------------------------------------------------------- 4. asset correlation
# A browser that renders a page pulls /styles/*.css. An IP that fetched HTML but
# never once fetched CSS is a headless client wearing a browser UA.
awk '$7 ~ /^\/styles\// {print $1}' "$WORK/maybe.log" | sort -u > "$WORK/rendered.txt"

grep -Ff "$WORK/rendered.txt" "$WORK/maybe.log" > "$WORK/human.log" || true

HTML='\.(css|js|png|jpe?g|gif|webp|ico|woff2?|wasm|svg|map)( |\?)'

echo "== traffic funnel =============================================="
printf '%7d  total requests\n'            "$(wc -l < "$WORK/all.log")"
printf '%7d  unique IPs\n'                "$(awk '{print $1}' "$WORK/all.log" | sort -u | wc -l)"
printf '%7d  exploit-scanner IPs dropped\n' "$(wc -l < "$WORK/scanners.txt")"
printf '%7d  requests after UA + scanner filter\n' "$(wc -l < "$WORK/maybe.log")"
printf '%7d  IPs that actually rendered CSS\n' "$(wc -l < "$WORK/rendered.txt")"
printf '%7d  human requests\n'            "$(wc -l < "$WORK/human.log")"

echo; echo "== human visitors (IP / hits / days seen / user-agent) ========="
awk -F'"' '{split($0,a," "); ip=a[1]; d=substr(a[4],2,11); ua=$6
  hits[ip]++; if (!seen[ip d]++) days[ip]++; agent[ip]=ua }
  END { for (i in hits) printf "%-16s %5d %4d  %.70s\n", i, hits[i], days[i], agent[i] }' \
  "$WORK/human.log" | sort -k2 -rn

# ---------------------------------------------------------------- 5. hosting filter
# Last step, not first: anything left that lives on a cloud/hosting network is a
# bot wearing a browser UA. Residential/mobile ISPs are what humans come from.
HOSTING='GOOGLE|AMAZON|AWS|MICROSOFT|AZURE|ORACLE|TENCENT|ALIBABA|ACEVILLE|OVH|HETZNER|DIGITALOCEAN|LINODE|AKAMAI|CLOUDFLARE|VULTR|LEASEWEB|CONTABO|SCALEWAY|CHOOPA|M247|DATACAMP|IONOS|NETCUP|QUADRANET|FDCSERVERS|INTERSERVER|ZENLAYER|CLOUVIDER|HOSTING|DATACENTER|COLO'
: > "$WORK/verdict.txt"
while read -r ip; do
  org=$(whois "$ip" 2>/dev/null | grep -iE 'OrgName:|org-name:|descr:' | head -1 | sed 's/.*: *//')
  if echo "$org" | grep -qiE "$HOSTING"; then v=hosting; else v=HUMAN; fi
  printf '%-16s %-8s %s\n' "$ip" "$v" "$org" >> "$WORK/verdict.txt"
done < "$WORK/rendered.txt"

echo; echo "== network owner verdict ======================================"
sort -k2 "$WORK/verdict.txt"

grep " HUMAN " "$WORK/verdict.txt" | awk '{print $1}' > "$WORK/final.txt"
grep -Ff "$WORK/final.txt" "$WORK/human.log" > "$WORK/final.log" || true

echo; echo "== pages humans read =========================================="
grep -vE "$HTML" "$WORK/final.log" | awk '{print $7}' \
  | sort | uniq -c | sort -rn | head -30

echo; echo "== external referrers ========================================="
awk -F'"' '{print $4}' "$WORK/final.log" \
  | grep -Ev '^-$|matthewritch\.com' | sort | uniq -c | sort -rn | head -15
