jQuery(document).ready(function(){
   // EFFETTI GANZI!
 	  //$('div.menu,div.login').corner();
 	  settings = {
          tl: { radius: 10 },
          tr: { radius: 10 },
          bl: { radius: 10 },
          br: { radius: 10 },
          antiAlias: true,
          autoPad: false
          
      }
      
      var divObj = document.getElementById("loginbox"); 

 	  var myBoxObject = new curvyCorners(settings, divObj);
      myBoxObject.applyCornersToAll();
 	/*$('div.spot,div.menu,div.login').corner();*/
 	/*$('div.top_box').corner();*/


 });
