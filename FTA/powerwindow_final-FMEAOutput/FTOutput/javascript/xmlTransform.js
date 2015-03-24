
function loadXMLdocument(FileName)
{
    var XmlDoc;
    // code for IE but still need to check the version of the msxml...
    //bothering, see this later on...
    if (window.ActiveXObject)
    {
      XmlDoc=new ActiveXObject("Microsoft.XMLDOM");
    }
    // code for Firefox, Mozilla, Opera? (may be), Netscape (Hopefully).
    else 
        if (document.implementation && document.implementation.createDocument)
        {
            XmlDoc=document.implementation.createDocument("","",null);
        }
        else
        {
            alert('Your browser cannot handle this script');
        }
        
    XmlDoc.async=false;
    XmlDoc.load(FileName);
    return(XmlDoc);
}

function transformIsland(xmlFile,xslFile,nodeTarget)
{	
    	
	xml=loadXMLdocument(xmlFile);
    xsl=loadXMLdocument(xslFile);
     // code for IE
    if (window.ActiveXObject)
    {
        ex = xml.transformNode(xsl);
      document.getElementById(nodeTarget).innerHTML=ex;
    }
    // code for Mozilla, Firefox, Opera, etc.
    else 
        if (document.implementation && document.implementation.createDocument)
        {
            xsltProcessor=new XSLTProcessor();
            xsltProcessor.importStylesheet(xsl);
            resultDocument = xsltProcessor.transformToFragment(xml,document);
            document.getElementById(nodeTarget).appendChild(resultDocument);            
        }            
}

function changeTitle(newTitle)
{
	document.title = newTitle;
}

 function transformCutSets(xmlFile, xslFile, ftName,sortType, sortOrder, myPageSize, myPage, myCutSetOrder) {
    var xslt = new ActiveXObject("Msxml2.XSLTemplate");
     var xslDoc = new ActiveXObject("Msxml2.FreeThreadedDOMDocument");
     var xslProc;
     xslDoc.async = false;
     xslDoc.resolveExternals = false;
     xslDoc.load(xslFile);
     xslt.stylesheet = xslDoc;
     var xmlDoc = new ActiveXObject("Msxml2.DOMDocument");
     xmlDoc.async = false;
     xmlDoc.resolveExternals = false;
     xmlDoc.load(xmlFile);
     xslProc = xslt.createProcessor();
     xslProc.input = xmlDoc;
     xslProc.addParameter("ftName", ftName);
     xslProc.addParameter("sortType", sortType);
     xslProc.addParameter("sortOrder", sortOrder);
     xslProc.addParameter("PageSize", Number(myPageSize));
     xslProc.addParameter("Page", Number(myPage));
     xslProc.addParameter("cutSetOrder", Number(myCutSetOrder));
     xslProc.transform();
     var scr = xslProc.output;

     ContentSpace.innerHTML = scr;

 }

 function transformFaultTree(xmlFile, xslFile, ftName) {
     var xslt = new ActiveXObject("Msxml2.XSLTemplate");
     var xslDoc = new ActiveXObject("Msxml2.FreeThreadedDOMDocument");
     var xslProc;
     xslDoc.async = false;
     xslDoc.resolveExternals = false;
     xslDoc.load(xslFile);
     xslt.stylesheet = xslDoc;
     var xmlDoc = new ActiveXObject("Msxml2.DOMDocument");
     xmlDoc.async = false;
     xmlDoc.resolveExternals = false;
     xmlDoc.load(xmlFile);
     xslProc = xslt.createProcessor();
     xslProc.input = xmlDoc;
     xslProc.addParameter("ftName", ftName);
     xslProc.transform();
     var scr = xslProc.output;

     ContentSpace.innerHTML = scr;

 }

 function transformTopEvent(xmlFile, xslFile, ftName) {
     var xslt = new ActiveXObject("Msxml2.XSLTemplate");
     var xslDoc = new ActiveXObject("Msxml2.FreeThreadedDOMDocument");
     var xslProc;
     xslDoc.async = false;
     xslDoc.resolveExternals = false;
     xslDoc.load(xslFile);
     xslt.stylesheet = xslDoc;
     var xmlDoc = new ActiveXObject("Msxml2.DOMDocument");
     xmlDoc.async = false;
     xmlDoc.resolveExternals = false;
     xmlDoc.load(xmlFile);
     xslProc = xslt.createProcessor();
     xslProc.input = xmlDoc;
     xslProc.addParameter("ftName", ftName);
     xslProc.transform();
     var scr = xslProc.output;

     OverviewSpace.innerHTML = scr;

 }

 function transformOverview(xmlFile, xslFile, sortOrder) {
     var xslt = new ActiveXObject("Msxml2.XSLTemplate");
     var xslDoc = new ActiveXObject("Msxml2.FreeThreadedDOMDocument");
     var xslProc;
     xslDoc.async = false;
     xslDoc.resolveExternals = false;
     xslDoc.load(xslFile);
     xslt.stylesheet = xslDoc;
     var xmlDoc = new ActiveXObject("Msxml2.DOMDocument");
     xmlDoc.async = false;
     xmlDoc.resolveExternals = false;
     xmlDoc.load(xmlFile);
     xslProc = xslt.createProcessor();
     xslProc.input = xmlDoc;
	 xslProc.addParameter("sortOrder", sortOrder);
     xslProc.transform();
     var scr = xslProc.output;

     ContentSpace.innerHTML = scr;
    // document.getElementById("ContentSpace").innerHTML = scr;

 }

 function transformMenus(xmlFile, xslFile) {
     var xslt = new ActiveXObject("Msxml2.XSLTemplate");
     var xslDoc = new ActiveXObject("Msxml2.FreeThreadedDOMDocument");
     var xslProc;
     xslDoc.async = false;
     xslDoc.resolveExternals = false;
     xslDoc.load(xslFile);
     xslt.stylesheet = xslDoc;
     var xmlDoc = new ActiveXObject("Msxml2.DOMDocument");
     xmlDoc.async = false;
     xmlDoc.resolveExternals = false;
     xmlDoc.load(xmlFile);
     xslProc = xslt.createProcessor();
     xslProc.input = xmlDoc;
     xslProc.transform();
     var scr = xslProc.output;

     MenuHeader.innerHTML = scr;

 }

function transformOptimisationIndividual(xmlFile, xslFile, individualID) {
    var xslt = new ActiveXObject("Msxml2.XSLTemplate");
    var xslDoc = new ActiveXObject("Msxml2.FreeThreadedDOMDocument");
    var xslProc;
    xslDoc.async = false;
    xslDoc.resolveExternals = false;
    xslDoc.load(xslFile);
    xslt.stylesheet = xslDoc;
    var xmlDoc = new ActiveXObject("Msxml2.DOMDocument");
    xmlDoc.async = false;
    xmlDoc.resolveExternals = false;
    xmlDoc.load(xmlFile);
    xslProc = xslt.createProcessor();
    xslProc.input = xmlDoc;
    xslProc.addParameter("xmlFile", xmlFile);
    xslProc.addParameter("IndividualIDParam", individualID);
    xslProc.transform();
    var scr = xslProc.output;

    ContentSpace.innerHTML = scr;

}

function transformOptimisationIndividualSummary(xmlFile, xslFile, individualID) {
    var xslt = new ActiveXObject("Msxml2.XSLTemplate");
    var xslDoc = new ActiveXObject("Msxml2.FreeThreadedDOMDocument");
    var xslProc;
    xslDoc.async = false;
    xslDoc.resolveExternals = false;
    xslDoc.load(xslFile);
    xslt.stylesheet = xslDoc;
    var xmlDoc = new ActiveXObject("Msxml2.DOMDocument");
    xmlDoc.async = false;
    xmlDoc.resolveExternals = false;
    xmlDoc.load(xmlFile);
    xslProc = xslt.createProcessor();
    xslProc.input = xmlDoc;
    xslProc.addParameter("xmlFile", xmlFile);
    xslProc.addParameter("IndividualIDParam", individualID);
    xslProc.transform();
    var scr = xslProc.output;

    OverviewSpace.innerHTML = scr;

}

function runTransform(xmlFileName, xslFileName, nodeToTransform, parameters) {
    if (document.implementation && document.implementation.createDocument && window.XSLTProcessor) {
        // Mozilla
	 var xsltProcessor = new XSLTProcessor();

        // load the xslt file
        var myXMLHTTPRequest = new XMLHttpRequest();
        myXMLHTTPRequest.open("GET", xslFileName, false);
        myXMLHTTPRequest.send(null);

        // get the XML document
        xslStylesheet = myXMLHTTPRequest.responseXML;
        xsltProcessor.importStylesheet(xslStylesheet);

        // load the xml file
        myXMLHTTPRequest = new XMLHttpRequest();
        myXMLHTTPRequest.open("GET", xmlFileName, false);
        myXMLHTTPRequest.send(null);

        var xmlSource = myXMLHTTPRequest.responseXML;
        if (parameters != "") {//Loop through the parameters and apply each to the XSLT Processor
            for (Index in parameters) {
               xsltProcessor.setParameter("", parameters[Index].name, parameters[Index].value);
            }
        }

        //transform
       var resultDocument = xsltProcessor.transformToFragment(xmlSource, document);
       var node = document.getElementById(nodeToTransform);
       while (node.hasChildNodes()) {
           node.removeChild(node.firstChild);
       }
       
       node.appendChild(resultDocument);
       
    } else if (window.ActiveXObject || "ActiveXObject" in window) {
        // IE

        var xslt = new ActiveXObject("Msxml2.XSLTemplate");
        var xslDoc = new ActiveXObject("Msxml2.FreeThreadedDOMDocument");
        var xslProc;
        xslDoc.async = false;
        xslDoc.resolveExternals = false;
        xslDoc.load(xslFileName);
        xslt.stylesheet = xslDoc;
        var xmlDoc = new ActiveXObject("Msxml2.DOMDocument");
        xmlDoc.async = false;
        xmlDoc.resolveExternals = false;
        xmlDoc.load(xmlFileName);
        xslProc = xslt.createProcessor();
        xslProc.input = xmlDoc;
       
        if (parameters != "") {//Loop through the parameters and apply each to the XSLT Processor
            for (Index in parameters) {
                xslProc.addParameter(parameters[Index].name, parameters[Index].value);
            }
        }
        xslProc.transform();
        var scr = xslProc.output;
        // Transform
        document.getElementById(nodeToTransform).innerHTML = scr;
    } else {
        // Browser unknown
        alert("Browser unknown");
    }

}

function menuTransform(xmlFileName, title) {
    var parameters = new Array();
    var parameter = new Object();
    parameter.name = 'XmlFileName';
    parameter.value = xmlFileName;
    parameters.push(parameter);
    
    parameter = new Object();
    parameter.name = 'Title';
    parameter.value = title;
    parameters.push(parameter);

    var nodeToTransform = 'MenuHeader';
    var xslFileName = 'FTOutput/XSL/menus.xsl';
    changeTitle(title);  
    runTransform(xmlFileName, xslFileName, nodeToTransform, parameters);
}

function warningsTransform(xmlFileName) {
    var parameters = new Array();
    var parameter = new Object();
    parameter.name = 'XmlFileName';
    parameter.value = xmlFileName;
    parameters.push(parameter);

    var nodeToTransform = 'ContentSpace';
    var xslFileName = 'FTOutput/XSL/warnings.xsl';
    clearNode('OverviewSpace');
    runTransform(xmlFileName, xslFileName, nodeToTransform, parameters);
}

function topEventTransform(xmlFileName, ftName) {
    var parameters = new Array();
    var parameter = new Object();
    parameter.name = 'XmlFileName';
    parameter.value = xmlFileName;
    parameters.push(parameter);

    parameter = new Object();
    parameter.name = 'ftName';
    parameter.value = ftName;
    parameters.push(parameter);

    var nodeToTransform = 'OverviewSpace';
    var xslFileName = 'FTOutput/XSL/topEventInfo.xsl';
    runTransform(xmlFileName, xslFileName, nodeToTransform, parameters);
}

function cutSetsTransform(xmlFileName, ftName, sortType, sortOrder, PageSize, Page, cutSetOrder, basicEventID) {
    var parameters = new Array();
    var parameter = new Object();
    parameter.name = 'XmlFileName';
    parameter.value = xmlFileName;
    parameters.push(parameter);

    parameter = new Object();
    parameter.name = 'ftName';
    parameter.value = ftName;
    parameters.push(parameter);

    parameter = new Object();
    parameter.name = 'sortType';
    parameter.value = sortType;
    parameters.push(parameter);

    parameter = new Object();
    parameter.name = 'sortOrder';
    parameter.value = sortOrder;
    parameters.push(parameter);

    parameter = new Object();
    parameter.name = 'PageSize';
    parameter.value = PageSize;
    parameters.push(parameter);

    parameter = new Object();
    parameter.name = 'Page';
    parameter.value = Page;
    parameters.push(parameter);

    parameter = new Object();
    parameter.name = 'cutSetOrder';
    parameter.value = cutSetOrder;
    parameters.push(parameter);
	
	parameter = new Object();
    parameter.name = 'basicEventID';
    parameter.value = basicEventID;
    parameters.push(parameter);

    var nodeToTransform = 'ContentSpace';
    var xslFileName = 'FTOutput/XSL/cutsets.xsl';
    runTransform(xmlFileName, xslFileName, nodeToTransform, parameters);
}
function optimisationSummaryTransform(xmlFileName) {
    var parameters = new Array();
    var parameter = new Object();
    parameter.name = 'XmlFileName';
    parameter.value = xmlFileName;
    parameters.push(parameter);

  

    var nodeToTransform = 'OverviewSpace';
    var xslFileName = 'FTOutput/XSL/optimisationSummary.xsl';
    clearNode('OverviewSpace');
    runTransform(xmlFileName, xslFileName, nodeToTransform, parameters);
}
function optimisationTransform(xmlFileName, sortType, sortOrder) {
    var parameters = new Array();
    var parameter = new Object();
    parameter.name = 'XmlFileName';
    parameter.value = xmlFileName;
    parameters.push(parameter);

    parameter = new Object();
    parameter.name = 'sortType';
    parameter.value = sortType;
    parameters.push(parameter);

    parameter = new Object();
    parameter.name = 'sortOrder';
    parameter.value = sortOrder;
    parameters.push(parameter);

    
    var nodeToTransform = 'ContentSpace';
    var xslFileName = 'FTOutput/XSL/optimisation.xsl';
    runTransform(xmlFileName, xslFileName, nodeToTransform, parameters);
}

function optimisationIndividualSummaryTransform(xmlFileName, individualID) {
    var parameters = new Array();
    var parameter = new Object();
    parameter.name = 'XmlFileName';
    parameter.value = xmlFileName;
    parameters.push(parameter);

    parameter = new Object();
    parameter.name = 'IndividualIDParam';
    parameter.value = individualID;
    parameters.push(parameter);


    var nodeToTransform = 'OverviewSpace';
    var xslFileName = 'FTOutput/XSL/optimisationSummary.xsl';
    runTransform(xmlFileName, xslFileName, nodeToTransform, parameters);
}
function optimisationIndividualTransform(xmlFileName, encodingType, individualID) {
    var parameters = new Array();
    var parameter = new Object();
    parameter.name = 'XmlFileName';
    parameter.value = xmlFileName;
    parameters.push(parameter);

    parameter = new Object();
    parameter.name = 'IndividualIDParam';
    parameter.value = individualID;
    parameters.push(parameter);


    var nodeToTransform = 'ContentSpace';
    var xslFileName = 'FTOutput/XSL/' + encodingType + '.xsl';
    runTransform(xmlFileName, xslFileName, nodeToTransform, parameters);
}

function fmeaTransform(xmlFileName, FMEAType, Page, PageSize) {
    var parameters = new Array();
    var parameter = new Object();
    parameter.name = 'XmlFileName';
    parameter.value = xmlFileName;
    parameters.push(parameter);

    parameter = new Object();
    parameter.name = 'FMEAType';
    parameter.value = FMEAType;
    parameters.push(parameter);

    parameter = new Object();
    parameter.name = 'Page';
    parameter.value = Page;
    parameters.push(parameter);

    parameter = new Object();
    parameter.name = 'PageSize';
    parameter.value = PageSize;
    parameters.push(parameter);

    var nodeToTransform = 'ContentSpace';
    var xslFileName = 'FTOutput/XSL/FMEA.xsl';
    clearNode('OverviewSpace');
    runTransform(xmlFileName, xslFileName, nodeToTransform, parameters);
}


function faultTreesTransform(xmlFileName, ftName) {
    var parameters = new Array();
    var parameter = new Object();
    parameter.name = 'XmlFileName';
    parameter.value = xmlFileName;
    parameters.push(parameter);

    parameter = new Object();
    parameter.name = 'ftName';
    parameter.value = ftName;
    parameters.push(parameter);

    var nodeToTransform = 'ContentSpace';
    var xslFileName = 'FTOutput/XSL/faultTreeView.xsl';
    runTransform(xmlFileName, xslFileName, nodeToTransform, parameters);
}

function overviewTransform(xmlFileName, sortOrder) {
    var parameters = new Array();
    var parameter = new Object();
    parameter.name = 'XmlFileName';
    parameter.value = xmlFileName;
    parameters.push(parameter);
	
	parameter = new Object();
    parameter.name = 'sortOrder';
    parameter.value = sortOrder;
    parameters.push(parameter);

    var nodeToTransform = 'ContentSpace';
    var xslFileName = 'FTOutput/XSL/overviewTable.xsl';
    clearNode('OverviewSpace');
    runTransform(xmlFileName, xslFileName, nodeToTransform, parameters);
}

function clearNode(nodeToClear) {
    if (document.implementation && document.implementation.createDocument) {
        // Mozilla

        //transform
        var node = document.getElementById(nodeToClear);
        while (node.hasChildNodes()) {
            node.removeChild(node.firstChild);
        }
    } else if (window.ActiveXObject || "ActiveXObject" in window) {
        // IE

       
        // Transform
        document.getElementById(nodeToClear).innerHTML = '';
    } else {
        // Browser unknown
        alert("Browser unknown");
    }
}