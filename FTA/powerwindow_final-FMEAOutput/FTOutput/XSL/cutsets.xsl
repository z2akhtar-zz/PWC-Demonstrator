<?xml version="1.0"?>
<xsl:stylesheet version="1.0"
      xmlns:faultTree ="http://www.hiphops.com/namespace/faultTree"
	  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"

>
  <xsl:param name="XmlFileName"/>
  <xsl:param name="ftName"/>
  <xsl:param name="sortType"/>
  <xsl:param name="sortOrder"/>
  <xsl:param name="PageSize" />
  <xsl:param name="Page" />
  <xsl:param name="cutSetOrder" />
  <xsl:param name="basicEventID" />

  <xsl:template match="HiP-HOPS_Results">
    <xsl:apply-templates select="FaultTrees" />
  </xsl:template>
  


  <xsl:template match="FaultTrees">
    <xsl:variable name="numCutSets">
		<xsl:choose>
			<xsl:when test="$basicEventID = 0" >
				<xsl:choose>
					<xsl:when test="$cutSetOrder = 0" >
						<xsl:value-of select="count(FaultTree[./Name=$ftName]/AllCutSets/CutSets/CutSet)"/>
					</xsl:when>
					<xsl:otherwise>
						<xsl:value-of select="count(FaultTree[./Name=$ftName]/AllCutSets/CutSets[@order=$cutSetOrder]/CutSet)"/>
					</xsl:otherwise>
				</xsl:choose>
			</xsl:when>
			<xsl:otherwise>
				<xsl:choose>
					<xsl:when test="$cutSetOrder = 0" >
						<xsl:value-of select="count(FaultTree[./Name=$ftName]/AllCutSets/CutSets/CutSet[./Events/Event/@ID=$basicEventID])"/>
					</xsl:when>
					<xsl:otherwise>
						<xsl:value-of select="count(FaultTree[./Name=$ftName]/AllCutSets/CutSets[@order=$cutSetOrder]/CutSet[./Events/Event/@ID=$basicEventID])"/>
					</xsl:otherwise>
				</xsl:choose>
			</xsl:otherwise>
		</xsl:choose>
    </xsl:variable>
    
    <xsl:variable name="numPages" select="($numCutSets) div $PageSize" />
    
    <div id="header">
      <ul id="primary">
        <li>
         <xsl:element name="a">
            <xsl:attribute name="href">
              javascript:
              topEventTransform('FaultTrees.xml','<xsl:value-of select="$ftName" />');
              faultTreesTransform('FaultTrees.xml','<xsl:value-of select="$ftName" />');
              menuTransform('FaultTrees.xml','Fault Tree View: <xsl:value-of select="$ftName" />');
            </xsl:attribute>
            Fault Tree
          </xsl:element>
        </li>
        <li>
          <span>Cut Sets</span>
          <ul id="secondary">
            <xsl:choose>

              <xsl:when test="$Page &gt; 0">

                <li>
                  <xsl:element name="a">
                    <xsl:attribute name="href">
                      javascript:
                      cutSetsTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="$ftName" />', '<xsl:value-of select="$sortType" />', '<xsl:value-of select="$sortOrder" />','<xsl:value-of select="$PageSize" />', '0', '<xsl:value-of select="$cutSetOrder" />', '<xsl:value-of select="$basicEventID" />');
                   
                    </xsl:attribute>
                    First Page
                  </xsl:element>
                </li>
              </xsl:when>
              <xsl:otherwise>
                <li>
                  <span>First Page</span>
                </li>
              </xsl:otherwise>
            </xsl:choose>
            <xsl:choose>
              
            <xsl:when test="$Page &gt; 0">
             
            <li>
              <xsl:element name="a">
                <xsl:attribute name="href">
                  javascript:
                  cutSetsTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="$ftName" />', '<xsl:value-of select="$sortType" />', '<xsl:value-of select="$sortOrder" />','<xsl:value-of select="$PageSize" />', '<xsl:value-of select="$Page - 1" />', '<xsl:value-of select="$cutSetOrder" />', '<xsl:value-of select="$basicEventID" />');

                </xsl:attribute>
                Previous Page
              </xsl:element>
            </li>
            </xsl:when>
              <xsl:otherwise>
                <li>
                  <span>Previous Page</span>
                </li>
              </xsl:otherwise>
            </xsl:choose>
            <li>
              <span>
                Current Page: <xsl:value-of select="$Page + 1"/> of <xsl:value-of select="ceiling($numPages)"/>
              </span>
            </li>
            <xsl:choose>
              <xsl:when test="$Page + 1 &lt; $numPages">
                <li>
                  <xsl:element name="a">
                    <xsl:attribute name="href">
                      javascript:
                      cutSetsTransform('<xsl:value-of select="$XmlFileName" />',
					  '<xsl:value-of select="$ftName" />', 
					  '<xsl:value-of select="$sortType" />', 
					  '<xsl:value-of select="$sortOrder" />',
					  '<xsl:value-of select="$PageSize" />', 
					  '<xsl:value-of select="$Page + 1" />', 
					  '<xsl:value-of select="$cutSetOrder" />', 
					  '<xsl:value-of select="$basicEventID" />');
                    </xsl:attribute>
                    Next Page
                  </xsl:element>
                </li>
              </xsl:when>
              <xsl:otherwise>
                <li>
                  <span>Next Page</span>
                </li>
              </xsl:otherwise>
            </xsl:choose>
            <xsl:choose>
                <xsl:when test="$Page + 1 &lt; $numPages">
                  <li>
                    <xsl:element name="a">
                      <xsl:attribute name="href">
                        javascript:
                        cutSetsTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="$ftName" />', '<xsl:value-of select="$sortType" />', '<xsl:value-of select="$sortOrder" />','<xsl:value-of select="$PageSize" />', '<xsl:value-of select="floor($numPages)" />', '<xsl:value-of select="$cutSetOrder" />', '<xsl:value-of select="$basicEventID" />');
                      </xsl:attribute>
                      Last Page
                    </xsl:element>
                  </li>
                </xsl:when>
                <xsl:otherwise>
                  <li>
                    <span>Last Page</span>
                  </li>
                </xsl:otherwise>
              </xsl:choose>
           
            
          </ul>
        </li>
        <li>
        Number of cut sets per page:
            <xsl:element name='select'>
              <xsl:attribute name='id'>PageSizeSelect</xsl:attribute>
              <xsl:attribute name='size'>1</xsl:attribute>
              <xsl:attribute name='onchange'>
			  
			  javascript:
						
						var newPageSize = document.getElementById('PageSizeSelect').options[document.getElementById('PageSizeSelect').selectedIndex].value;
						var newNumPages = Math.ceil(<xsl:value-of select="$numCutSets"/> / newPageSize);
						var newPage = <xsl:value-of select="$Page"/> ;
						if(newPage + 1 &gt; newNumPages)
						{
							newPage = newNumPages - 1;
						}
						
						cutSetsTransform('<xsl:value-of select="$XmlFileName" />',
						'<xsl:value-of select="$ftName" />', 
						'<xsl:value-of select="$sortType" />', 
						'<xsl:value-of select="$sortOrder" />',
						newPageSize,
						newPage, 
						'<xsl:value-of select="$cutSetOrder" />',
						'<xsl:value-of select="$basicEventID" />');
                            
              </xsl:attribute>
              <xsl:element name='option'>
                <xsl:attribute name='value'>100</xsl:attribute>
                <xsl:if test='$PageSize = 100'>
                  <xsl:attribute name='selected' />
                </xsl:if>
                100
              </xsl:element>
              <xsl:element name='option'>
                <xsl:attribute name='value'>200</xsl:attribute>
                <xsl:if test='$PageSize = 200'>
                  <xsl:attribute name='selected' />
                </xsl:if>
                200
              </xsl:element>
              <xsl:element name='option'>
                <xsl:attribute name='value'>400</xsl:attribute>
                <xsl:if test='$PageSize = 400'>
                  <xsl:attribute name='selected' />
                </xsl:if>
                400
              </xsl:element>
              <xsl:element name='option'>
                <xsl:if test='($PageSize = $numCutSets)'>
                  <xsl:attribute name='selected' />
                </xsl:if>
                <xsl:attribute name='value'>
                  <xsl:value-of select='$numCutSets'/>
                </xsl:attribute>
                All
              </xsl:element>

            </xsl:element>
          
        </li>
      </ul>
    </div>

    <div id="main">
    <TABLE id="ContentTable" cellspacing="0">
      <TR>
        <TD>
            
              <xsl:choose>
                <xsl:when test="$ftName=''">
                  <Script>alert('FaultTree not selected');</Script>

                </xsl:when>
                <xsl:otherwise>
                  
                  <xsl:apply-templates select="FaultTree[./Name=$ftName]" />
                  
                </xsl:otherwise>

              </xsl:choose>

        </TD>
      </TR>
    </TABLE>
    </div>
  </xsl:template>


  <xsl:template match="FaultTree">
    
    <xsl:apply-templates select="AllCutSets" />   

  </xsl:template>
  <xsl:template match="AllCutSets">
    <Table id="CutSetTable" cellSpacing="0">
    
      <xsl:choose>
        <xsl:when test="$sortType = 'order'">
          <xsl:apply-templates select="CutSets[./CutSet]" />

        </xsl:when>
        <xsl:otherwise>
          <xsl:variable name="numCurrentCutSets">
          <xsl:choose>
            <xsl:when test="$basicEventID = 0">
				<xsl:choose>
            <xsl:when test="$cutSetOrder = 0">
				<xsl:value-of select="count(CutSets/CutSet)"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="count(CutSets[@order = $cutSetOrder]/CutSet)"/>
            </xsl:otherwise>
          </xsl:choose>
            </xsl:when>
            <xsl:otherwise>
              <xsl:choose>
            <xsl:when test="$cutSetOrder = 0">
				<xsl:value-of select="count(CutSets/CutSet[./Events/Event/@ID=$basicEventID])"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="count(CutSets[@order = $cutSetOrder]/CutSet[./Events/Event/@ID=$basicEventID])"/>
            </xsl:otherwise>
          </xsl:choose>
            </xsl:otherwise>
          </xsl:choose>
          </xsl:variable>
          <xsl:variable name="numCutSetsIncCurrent" select="$numCurrentCutSets"/>
          <xsl:variable name="lowerBound" select="($Page * $PageSize) + 1"/>
          <xsl:variable name="upperBound" select="$PageSize + ($PageSize * $Page)"/>
          <xsl:variable name="lowerCutSet" select="$lowerBound"/>
          <xsl:variable name="upperCutSet" select="$lowerCutSet + $PageSize - 1"/>
          
          <Tr class="HeaderRow">
            <Td>
              Showing
              <xsl:choose>
                <xsl:when test="$lowerCutSet &lt; 1">1</xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="$lowerCutSet"/>
                </xsl:otherwise>
              </xsl:choose> to <xsl:choose>
                <xsl:when test="$upperCutSet &gt; $numCurrentCutSets">
                  <xsl:value-of select="$numCurrentCutSets"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:value-of select="$upperCutSet"/>
                </xsl:otherwise>
              </xsl:choose> of <xsl:value-of select="$numCurrentCutSets"/>
              Cut Sets 
              <xsl:choose>
                <xsl:when test="$cutSetOrder = 0">
              <xsl:element name="a">
              <xsl:attribute name="href">
                javascript:
                cutSetsTransform('<xsl:value-of select="$XmlFileName" />',
				'<xsl:value-of select="$ftName" />', 
				'order', 
				'<xsl:value-of select="$sortOrder" />',
				'<xsl:value-of select="$PageSize" />', 
				'<xsl:value-of select="$Page" />', 
				'<xsl:value-of select="$cutSetOrder" />', 
				'<xsl:value-of select="$basicEventID" />');
              </xsl:attribute>
              (Sort by Order)
            </xsl:element>
                </xsl:when>
                <xsl:otherwise>
                  of Order <xsl:value-of select='$cutSetOrder'/>
                </xsl:otherwise>
              </xsl:choose>
            </Td>
            
            <Td>Unavailability
			(
			<xsl:choose>
			<xsl:when test="$sortOrder = 'ascending'">ascending</xsl:when>
			<xsl:otherwise>
			<xsl:element name="a">
              <xsl:attribute name="href">
                javascript:
                cutSetsTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="$ftName" />', '<xsl:value-of select="$sortType" />', 'ascending','<xsl:value-of select="$PageSize" />', '<xsl:value-of select="$Page" />', '<xsl:value-of select="$cutSetOrder" />', '<xsl:value-of select="$basicEventID" />');
              </xsl:attribute>ascending</xsl:element>
			</xsl:otherwise>
			</xsl:choose>
			/
			<xsl:choose>
			<xsl:when test="$sortOrder = 'descending'">descending</xsl:when>
			<xsl:otherwise>
			<xsl:element name="a">
              <xsl:attribute name="href">
                javascript:
                cutSetsTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="$ftName" />', '<xsl:value-of select="$sortType" />', 'descending','<xsl:value-of select="$PageSize" />', '<xsl:value-of select="$Page" />', '<xsl:value-of select="$cutSetOrder" />', '<xsl:value-of select="$basicEventID" />');
              </xsl:attribute>descending</xsl:element>
			</xsl:otherwise>
			</xsl:choose>
			)</Td>
          </Tr>
				<xsl:choose>
					<xsl:when test='$basicEventID = 0'>
						<xsl:choose>
					<xsl:when test='$cutSetOrder = 0'>
						<xsl:apply-templates select="CutSets/CutSet">

						<xsl:sort select="UnavailabilitySort" order="{$sortOrder}" data-type="number" />
						<xsl:with-param name="numberOtherCutSets" select="0" />


						</xsl:apply-templates>
					</xsl:when>
					<xsl:otherwise>
						<xsl:apply-templates select="CutSets[@order = $cutSetOrder]/CutSet">

						<xsl:sort select="UnavailabilitySort" order="{$sortOrder}" data-type="number" />
						<xsl:with-param name="numberOtherCutSets" select="0" />


						</xsl:apply-templates>
					</xsl:otherwise>
				</xsl:choose>
					</xsl:when>
					<xsl:otherwise>
						<xsl:choose>
					<xsl:when test='$cutSetOrder = 0'>
						<xsl:apply-templates select="CutSets/CutSet[./Events/Event/@ID=$basicEventID]">

						<xsl:sort select="UnavailabilitySort" order="{$sortOrder}" data-type="number" />
						<xsl:with-param name="numberOtherCutSets" select="0" />


						</xsl:apply-templates>
					</xsl:when>
					<xsl:otherwise>
						<xsl:apply-templates select="CutSets[@order = $cutSetOrder]/CutSet[./Events/Event/@ID=$basicEventID]">

						<xsl:sort select="UnavailabilitySort" order="{$sortOrder}" data-type="number" />
						<xsl:with-param name="numberOtherCutSets" select="0" />


						</xsl:apply-templates>
					</xsl:otherwise>
				</xsl:choose>
					</xsl:otherwise>
				</xsl:choose>
        </xsl:otherwise>

      </xsl:choose>
    
</Table>
  </xsl:template>
  
  <xsl:template match="CutSets">
    <xsl:variable name="currentOrder" select="@order"/>
    <xsl:variable name="numPreviousCutSets">
		<xsl:choose>
			<xsl:when test="$basicEventID = 0" >
				<xsl:value-of select="count(../CutSets[@order &lt; $currentOrder]/CutSet)"/>
			</xsl:when>
			<xsl:otherwise>
				<xsl:value-of select="count(../CutSets[@order &lt; $currentOrder]/CutSet[./Events/Event/@ID=$basicEventID])"/>
			</xsl:otherwise>
		</xsl:choose>
    </xsl:variable>
    <xsl:variable name="lowerBound" select="($Page * $PageSize) + 1"/>
    <xsl:choose>
      <xsl:when test="$cutSetOrder = $currentOrder">
        <xsl:variable name="lowerCutSet" select="$lowerBound"/>
        <xsl:call-template name="CutSetsFunc">
          <xsl:with-param name ="numPreviousCutSets" select="0" />
          <xsl:with-param name ="lowerCutSet" select="$lowerCutSet" />
        </xsl:call-template>
      </xsl:when>
      <xsl:when test="$cutSetOrder = 0">
        
        <xsl:variable name="lowerCutSet" select="$lowerBound - $numPreviousCutSets"/>
        <xsl:call-template name="CutSetsFunc">
          <xsl:with-param name ="numPreviousCutSets" select="$numPreviousCutSets" />
          <xsl:with-param name ="lowerCutSet" select="$lowerCutSet" />
        </xsl:call-template>
        
      </xsl:when>
    </xsl:choose>
   
    

  </xsl:template>

  <xsl:template name="CutSetsFunc">
    <xsl:param name="numPreviousCutSets"/>
    <xsl:param name="lowerCutSet"/>
    <xsl:variable name="numCurrentCutSets">
		<xsl:choose>
			<xsl:when test="$basicEventID = 0" >
				<xsl:value-of select="count(CutSet)"/>
			</xsl:when>
			<xsl:otherwise>
				<xsl:value-of select="count(CutSet[./Events/Event/@ID=$basicEventID])"/>
			</xsl:otherwise>
		</xsl:choose>
    </xsl:variable>
    <xsl:variable name="numCutSetsIncCurrent" select="$numPreviousCutSets + $numCurrentCutSets"/>
    <xsl:variable name="lowerBound" select="($Page * $PageSize)+1"/>
    <xsl:variable name="upperBound" select="$PageSize + ($PageSize * $Page)"/>
    <xsl:variable name="upperCutSet" select="$lowerCutSet + $PageSize - 1"/>
    <xsl:if test="$numCutSetsIncCurrent &gt;= $lowerBound">
      <xsl:if test="$numPreviousCutSets &lt;= $upperBound">
        <Tr class="HeaderRow">
          <Td>
            Showing
            <xsl:choose>
              <xsl:when test="$lowerCutSet &lt; 1">1</xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="$lowerCutSet"/>
              </xsl:otherwise>
            </xsl:choose> to <xsl:choose>
              <xsl:when test="$upperCutSet &gt; $numCurrentCutSets">
                <xsl:value-of select="$numCurrentCutSets"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="$upperCutSet"/>
              </xsl:otherwise>
            </xsl:choose> of <xsl:value-of select="$numCurrentCutSets"/>
            Cut Sets of Order <xsl:value-of select="@order" />
          </Td>
          <Td>
            <xsl:element name="a">
              <xsl:attribute name="href">
                javascript:
                cutSetsTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="$ftName" />', 'unavailability', '<xsl:value-of select="$sortOrder" />','<xsl:value-of select="$PageSize" />', '<xsl:value-of select="$Page" />', '<xsl:value-of select="$cutSetOrder" />', '<xsl:value-of select="$basicEventID" />');
              </xsl:attribute>
              Unavailability
            </xsl:element>
			(
			<xsl:choose>
			<xsl:when test="$sortOrder = 'ascending'">ascending</xsl:when>
			<xsl:otherwise>
			<xsl:element name="a">
              <xsl:attribute name="href">
                javascript:
                cutSetsTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="$ftName" />', '<xsl:value-of select="$sortType" />', 'ascending','<xsl:value-of select="$PageSize" />', '<xsl:value-of select="$Page" />', '<xsl:value-of select="$cutSetOrder" />', '<xsl:value-of select="$basicEventID" />');
              </xsl:attribute>ascending</xsl:element>
			</xsl:otherwise>
			</xsl:choose>
			/
			<xsl:choose>
			<xsl:when test="$sortOrder = 'descending'">descending</xsl:when>
			<xsl:otherwise>
			<xsl:element name="a">
              <xsl:attribute name="href">
                javascript:
                cutSetsTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="$ftName" />', '<xsl:value-of select="$sortType" />', 'descending','<xsl:value-of select="$PageSize" />', '<xsl:value-of select="$Page" />', '<xsl:value-of select="$cutSetOrder" />', '<xsl:value-of select="$basicEventID" />');
              </xsl:attribute>descending</xsl:element>
			</xsl:otherwise>
			</xsl:choose>
			)

          </Td>
        </Tr>
		<xsl:choose>
		<xsl:when test="$basicEventID = 0">
			<xsl:apply-templates select="CutSet">
			<xsl:sort select="UnavailabilitySort" order="{$sortOrder}" data-type="number" />
			  <xsl:with-param name="numberOtherCutSets" select="$numPreviousCutSets" />
			</xsl:apply-templates>
		</xsl:when>
		<xsl:otherwise>
			<xsl:apply-templates select="CutSet[./Events/Event/@ID=$basicEventID]">
			<xsl:sort select="UnavailabilitySort" order="{$sortOrder}" data-type="number" />
			  <xsl:with-param name="numberOtherCutSets" select="$numPreviousCutSets" />
			</xsl:apply-templates>
		</xsl:otherwise>
		</xsl:choose>
      </xsl:if>
    </xsl:if>
  </xsl:template>

  <xsl:template match="CutSet">
    <xsl:param name="numberOtherCutSets"/>
    <xsl:variable name="currentPosition" select="$numberOtherCutSets + position()" />
    
    <xsl:if test="$currentPosition &gt;= ($Page * $PageSize) + 1">
    <xsl:if test="$currentPosition &lt;= $PageSize + ($PageSize * $Page)">

      <Tr class="DataRow">
        <Td>
          <xsl:apply-templates select="Events" />
        </Td>
        <Td>
          <xsl:value-of select="Unavailability"/>
        </Td>
      </Tr>
    </xsl:if>
    </xsl:if>


  </xsl:template>
  <xsl:template match="Events">
    <Table id="EventsTable">
      <xsl:apply-templates select="Event" />
      
    </Table>



  </xsl:template>

  

  <xsl:template match="Event">
    <xsl:variable name="eventID" select="@ID"/>
    <xsl:apply-templates select="/HiP-HOPS_Results/FaultTrees/FMEA/Component/Events/*[@ID=$eventID]">
    </xsl:apply-templates>

  </xsl:template>
  <xsl:template match="BasicEvent">
    <Tr>
      <Td>
        <DIV class="FailureModesCell">
          <xsl:element name="IMG">
            <xsl:attribute name="src">FTOutput/images/failureMode.gif</xsl:attribute>
          </xsl:element>
          <xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)
        </DIV>
      </Td>
    </Tr>
  </xsl:template>

  <xsl:template match="InputDeviationNode">
    <Tr>
      <Td>
        <DIV class="FailureModesCell">
          <xsl:element name="IMG">
            <xsl:attribute name="src">FTOutput/images/inputdeviation.gif</xsl:attribute>
          </xsl:element>
          <xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)
        </DIV>
      </Td>
    </Tr>
  </xsl:template>

  <xsl:template match="OutputDeviationNode">
    <Tr>
      <Td>
        <DIV class="FailureModesCell">
          <xsl:element name="IMG">
            <xsl:attribute name="src">FTOutput/images/outputdeviation.gif</xsl:attribute>
          </xsl:element>
          <xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)
        </DIV>
      </Td>
    </Tr>
  </xsl:template>

  <xsl:template match="ExportedDeviationNode">
    <Tr>
      <Td>
        <DIV class="FailureModesCell">
          <xsl:element name="IMG">
            <xsl:attribute name="src">FTOutput/images/outputdeviation.gif</xsl:attribute>
          </xsl:element>
          <xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)
        </DIV>
      </Td>
    </Tr>
  </xsl:template>

  <xsl:template match="PCCFNode">
    <Tr>
      <Td>
        <DIV class="FailureModesCell">
          <xsl:element name="IMG">
            <xsl:attribute name="src">FTOutput/images/proxy.gif</xsl:attribute>
          </xsl:element>
          <xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)
        </DIV>
      </Td>
    </Tr>
  </xsl:template>
  
   <xsl:template match="NormalEvent">
  <Tr>
      <Td>
        <DIV class="FailureModesCell">
          <xsl:element name="IMG">
            <xsl:attribute name="src">FTOutput/images/normalEvent.gif</xsl:attribute>
          </xsl:element>
          <xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)
        </DIV>
      </Td>
    </Tr>
  </xsl:template>
  <xsl:template match="InputDeviation">
    <Tr>
      <Td>
        <DIV class="FailureModesCell">
          <xsl:element name="IMG">
            <xsl:attribute name="src">FTOutput/images/inputdeviation.gif</xsl:attribute>
          </xsl:element>
          <xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)
        </DIV>
      </Td>
    </Tr>
  </xsl:template>
  <xsl:template match="OutputDeviation">
    <Tr>
      <Td>
        <DIV class="FailureModesCell">
          <xsl:element name="IMG">
            <xsl:attribute name="src">FTOutput/images/OutputDeviation.gif</xsl:attribute>
          </xsl:element>
          <xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)
        </DIV>
      </Td>
    </Tr>
  </xsl:template>

  <xsl:template match="Circle">
    <Tr>
      <Td>
        <DIV class="FailureModesCell">
          <xsl:element name="IMG">
            <xsl:attribute name="src">FTOutput/images/Circle.gif</xsl:attribute>
          </xsl:element>
          <xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)
        </DIV>
      </Td>
    </Tr>
  </xsl:template>

</xsl:stylesheet>