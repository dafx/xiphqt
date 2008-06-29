var opwindowCommon={
	openFileWindowDialog:function()
        {
		alert("Inside openFileWindowDialog");
		try{
                	const nsIFilePicker = Components.interfaces.nsIFilePicker;
	                var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
        	        fp.init(window, "Choose a File", nsIFilePicker.modeOpen);
                	fp.appendFilters(nsIFilePicker.filterAll | nsIFilePicker.filterText);
		}	
		catch(err){
			alert(err);
			return;
		}
                var rv = fp.show();
                if (rv == nsIFilePicker.returnOK || rv == nsIFilePicker.returnReplace) {
                	var file = fp.file;
         		var path = fp.file.path;
			parent.document.getElementById("file-path").value = path;
                }
         },
	 myComponentTestGo:function()
	 {
		//alert("Start of MyComponentTestGo()");
        	try {
               		netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
                	const cid = "@mydomain.com/XPCOMSample/MyComponent;1";
                	obj = Components.classes[cid].createInstance();
                	obj = obj.QueryInterface(Components.interfaces.IMyComponent);
        	} 
		catch (err) 
		{
	               	alert(err);
        	        return;
        	}
        	var res = obj.Add(3, 4);
        	alert('Performing 3+4. Returned ' + res + '.');
	},
	uploadFile:function()
	{
		try{
			var filePath = document.getElementById("file-path").value;
			var file = Components.classes['@mozilla.org/file/local;1'].createInstance(Components.interfaces.nsILocalFile);
			file.initWithPath(filePath);
			var fstream = Components.classes['@mozilla.org/network/file-input-stream;1'].createInstance(Components.interfaces.nsIFileInputStream);
			fstream.init(file, 1, 1, Components.interfaces.nsIFileInputStream.CLOSE_ON_EOF);
			var bstream = Components.classes['@mozilla.org/network/buffered-input-stream;1'].createInstance(Components.interfaces.nsIBufferedInputStream);
			bstream.init(fstream, 4096);
			xhr = new XMLHttpRequest();
		}
		catch(err){
			alert(err);
			return;
		}
	        xhr.onreadystatechange = function() { if(this.readyState == 4) {alert(this.responseText)}}
		xhr.open("POST", "http://localhost/upload.php", true);
		xhr.setRequestHeader('Content-Type', 'application/octet-stream');
		xhr.send(bstream);
		

	}
	

}

