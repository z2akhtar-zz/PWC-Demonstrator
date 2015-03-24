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

  <xsl:template match="HiP-HOPS_Results">
    <xsl:apply-templates select="FaultTrees" />
  </xsl:template>
  
  <xsl:template match="FaultTrees">
    <xsl:variable name="numPages" select="count(FaultTree[./Name=$ftName]/AllCutSets/CutSets/CutSet) div $PageSize" />
    

    <TABLE id="TopEventDataTable" cellspacing="1">
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
    
  </xsl:template>


  <xsl:template match="FaultTree">

    <Tr>
      <Td class="TopEventsLabel">
        Top Event (Effect)
      </Td>
      <Td class="TopEventsData">
        <xsl:value-of select="Name"/>
      </Td>
    </Tr>
    <Tr>
      <Td class="TopEventsLabel">
        Description
      </Td>
      <Td class="TopEventsData">
        <xsl:variable name="string" select="Description" />
        <xsl:variable name="str-length" select="string-length($string)" />
        <xsl:choose>
          <xsl:when test="$str-length != 0">
            <xsl:value-of select="$string" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>N/A</xsl:text>
          </xsl:otherwise>
        </xsl:choose>
      </Td>
    </Tr>
    <xsl:if test="Unavailability">
      <Tr>
        <Td class="TopEventsLabel">
          System Unavailability
        </Td>
        <Td class="TopEventsData">
          <xsl:value-of select="Unavailability" />
        </Td>
      </Tr>
    </xsl:if>
    <xsl:if test="Severity">
      <Tr>
        <Td class="TopEventsLabel">
          Severity
        </Td>
        <Td class="TopEventsData">
          <xsl:value-of select="Severity" />
        </Td>
      </Tr>
    </xsl:if>
    

  </xsl:template>
  <xsl:template match="AllCutSets">
    <Table id="CutSetTable" cellSpacing="0">
    
      <xsl:choose>
        <xsl:when test="$sortType = 'order'">
          <xsl:apply-templates select="CutSets" />

        </xsl:when>
        <xsl:otherwise>
          <xsl:variable name="numCurrentCutSets" select="count(CutSets/CutSet)"/>
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
              Cut Sets <xsl:element name="a">
              <xsl:attribute name="href">
                javascript:transformCutSets('<xsl:value-of select="$XmlFileName" />','FTOutput/XSL/ftcutsets.xsl', 'Omission-SteeringBrake.Out1', 'order', 'ascending', <xsl:value-of select='$PageSize' />, <xsl:value-of select='0' />, <xsl:value-of select='$cutSetOrder' />);
              </xsl:attribute>
              (Sort by Order)
            </xsl:element>
            </Td>
            
            <Td>Unavailability</Td>
          </Tr>
          <xsl:apply-templates select="CutSets/CutSet">

            <xsl:sort select="Unavailability" order="{$sortOrder}" data-type="number" />
            <xsl:with-param name="numberOtherCutSets" select="0" />
              
            
          </xsl:apply-templates> 

        </xsl:otherwise>

      </xsl:choose>
    
</Table>
  </xsl:template>
  
  <xsl:template match="CutSets">
    <xsl:variable name="currentOrder" select="@order"/>
    <xsl:variable name="numPreviousCutSets" select="count(../CutSets[@order &lt; $currentOrder]/CutSet)"/>
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
    <xsl:variable name="numCurrentCutSets" select="count(CutSet)"/>
    <xsl:variable name="numCutSetsIncCurrent" select="$numPreviousCutSets + $numCurrentCutSets"/>
    <xsl:variable name="lowerBound" select="($Page * $PageSize) + 1"/>
    <xsl:variable name="upperBound" select="$PageSize + ($PageSize * $Page)"/>
    <xsl:variable name="upperCutSet" select="$lowerCutSet + $PageSize - 1"/>
    <xsl:if test="$numCutSetsIncCurrent &gt; $lowerBound">
      <xsl:if test="$numPreviousCutSets &lt; $upperBound">
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
                javascript:transformCutSets('<xsl:value-of select="$XmlFileName" />','FTOutput/XSL/ftcutsets.xsl', 'Omission-SteeringBrake.Out1', 'unavailability', 'ascending', <xsl:value-of select='$PageSize' />, <xsl:value-of select='0' />, <xsl:value-of select='$cutSetOrder' />);
              </xsl:attribute>
              Unavailability
            </xsl:element>

          </Td>
        </Tr>
        <xsl:apply-templates select="CutSet">
          <xsl:with-param name="numberOtherCutSets" select="$numPreviousCutSets" />
        </xsl:apply-templates>
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
    <xsl:apply-templates select="/FaultTrees/Events/*[@ID=$eventID]">
    </xsl:apply-templates>

  </xsl:template>
  <xsl:template match="BasicEvent">

    <Tr>
      <Td>
        <DIV class="FailureModesCell">
          <xsl:element name="IMG">
            <xsl:attribute name="src">
              FTOutput/images/failureMode.gif
            </xsl:attribute>
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
            <xsl:attribute name="src">
              FTOutput/images/inputdeviation.gif
            </xsl:attribute>
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
            <xsl:attribute name="src">
              FTOutput/images/OutputDeviation.gif
            </xsl:attribute>
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
            <xsl:attribute name="src">
              FTOutput/images/Circle.gif
            </xsl:attribute>
          </xsl:element>
          <xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)
        </DIV>
      </Td>
    </Tr>

  </xsl:template>

</xsl:stylesheet>