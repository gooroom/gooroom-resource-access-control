function showThread( targetName )
{
	var oTarget = document.getElementById(targetName);
	oTarget.style.display = ( oTarget.style.display == 'none' )?
	'':'none';
            
	return null;
 }

function showMore(id) {
        var obj = document.getElementById(id);
        if (obj.style.display == 'none')
            obj.style.display = 'inline-block';
        else
            obj.style.display = 'none';
}

window.onload = function() {
  var ulTag = document.getElementsByTagName("ul");

  if (ulTag == null || ulTag == "undefined")
    return;

  var liTag = ulTag[0].children[1];

  if (liTag == null || liTag == "undefined")
    return;

  var aTag = document.createElement("a");
  aTag.href = "../index/index";
  aTag.text = "구름 도움말";

  var imgTag = document.createElement("img");
  imgTag.className = "img";
  imgTag.src = "grey.svg";

  var liTag1 = document.createElement("li");
  liTag1.appendChild (aTag);
  liTag1.appendChild (imgTag);

  ulTag[0].insertBefore(liTag1, liTag);
}
