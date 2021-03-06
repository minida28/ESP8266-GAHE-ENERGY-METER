<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    <xsl:template match="/">
        <html>
            <body>
                <h3>Parameters</h3>
                <table border="1">
                    <tr bgcolor="#9acd32">
                        <th>Variable</th>
                        <th>Value</th>
                    </tr>
                    <xsl:for-each select="xml/*">
                        <tr>
                            <td><xsl:value-of select="local-name()"/></td>
                            <td><xsl:value-of select="."/></td>
                        </tr>
                    </xsl:for-each>

                </table>
            </body>
        </html>
    </xsl:template>
</xsl:stylesheet>
