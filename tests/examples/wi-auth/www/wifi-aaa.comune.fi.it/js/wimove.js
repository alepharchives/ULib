(function(w, d) {
var wimoveGetQueryVar = this.wimoveGetQueryVar = function(variable) {
  var query = window.location.search.substring(1); 
  var vars = query.split("&"); 
  for (var i=0;i<vars.length;i++) { 
    var pair = vars[i].split("="); 
    if (pair[0] == variable) { 
      return unescape(pair[1]); 
    } 
  } 
};

var wimoveFetchUrl = this.wimoveFetchUrl = function(url) {
  var request = false;
  if (window.XMLHttpRequest) {
    request = new XMLHttpRequest;
  } else if (window.ActiveXObject) {
    request = new ActiveXObject("Microsoft.XMLHttp");
  }
  if (request) {
    request.open("GET", url, false);
    request.send();
    if (request.status == 200) { return request.responseText; }
  }
  return false;
};

var wimoveBuildBanner = this.wimoveBuildBanner =  function(baseUrl) {
  var gateway=wimoveGetQueryVar("ap");
  if (!gateway) {
    gateway='default';
  }
  var bannerUrl = baseUrl + '/' + gateway + (isMobile()?'/mobile':'/full') + '/banner.html';
  var banner;
  if (gateway !== 'default') {
    banner = wimoveFetchUrl(bannerUrl);
    if (banner === false) {
          bannerUrl = baseUrl + '/default' + (isMobile()?'/mobile':'/full') + '/banner.html';
          banner = wimoveFetchUrl(bannerUrl);
    }
  }
  if (banner !== false) {
    document.write(banner);
  }
};
})(this, this.document);
