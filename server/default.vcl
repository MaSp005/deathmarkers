# Adapted from https://github.com/mattiasgeniar/varnish-6.0-configuration-templates/blob/master/default.vcl
vcl 4.1;

backend default {
  .host = "deathmarkers";
  .port = "8048";
}

sub vcl_recv {
  set req.backend_hint = default;

  if (req.method != "GET" &&
      req.method != "HEAD" &&
      req.method != "PUT" &&
      req.method != "POST" &&
      req.method != "TRACE" &&
      req.method != "OPTIONS" &&
      req.method != "PATCH" &&
      req.method != "DELETE") {
    return (pipe);
  }

  # Only cache GET or HEAD requests. This makes sure the POST requests are always passed.
  if (req.method != "GET") {
    return (pipe);
  }

  if (!(req.url ~ "^\/list\/?\?")) {
    return (pipe);
  }

  # Strip hash, server doesn't need it.
  if (req.url ~ "\#") {
    set req.url = regsub(req.url, "\#.*$", "");
  }

  # Strip a trailing ? if it exists
  if (req.url ~ "\?$") {
    set req.url = regsub(req.url, "\?$", "");
  }

  return (hash);
}

sub vcl_hash {
  hash_data(req.url);
}

sub vcl_hit {
  # Called when a cache lookup is successful.

  if (obj.ttl >= 0s) {
    return (deliver);
  }
}

sub vcl_miss {
  return (fetch);
}

sub vcl_backend_response {
  set beresp.do_stream = true;

  set beresp.ttl = 120s;

  # Don't cache 50x responses
  if (beresp.status == 500 || beresp.status == 502 || beresp.status == 503 || beresp.status == 504) {
    return (abandon);
  }

  set beresp.grace = 240s;

  return (deliver);
}
