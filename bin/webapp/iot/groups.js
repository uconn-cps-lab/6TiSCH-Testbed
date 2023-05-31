/*
 * Copyright (c) 2015-2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


var iot = iot || { };

jQuery(document).ready(function() {
  $.ajax({
    url: "node/groups.json",
    error: function(e) {
      console.log("error");
    },
    fail: function(e) {
      console.log("fail");
    },
    success: function(d) {
      iot.groups = d;
      
      var buf = '';
      
      buf += '<table>';
      buf += '<tr class="table_header"><td>';
      buf += 'GROUP NAME';
      buf += '</td><td>';
      buf += 'DESCRIPTION';
      buf += '</td><td>';
      buf += 'NODES';
      buf += '</td><td>';
      buf += 'ACTIONS';
      buf += '</td></tr>';
      
      for (i = 0; i < iot.groups.length; i++) {
        buf += '<tr><td>';
        buf += iot.groups[i]["name"] + ' (' + iot.groups[i]["id"] + ')';
        buf += '</td><td>';
        buf += iot.groups[i]["description"];
        buf += '</td><td>';
        buf += iot.groups[i].nodes.length;
        buf += '</td><td>';
        buf += '<span>&nbsp;Edit&nbsp;</span><span>&nbsp;Delete&nbsp;</span>';
        buf += '</td></tr>';
      }
      
      buf += '<tr><td colspan="4">';
      buf += '<form action="#">';
      buf += '<label for="create_group"></label>';
      buf += '<input type="text" id="create_group" name="create_group" />';
      buf += '&nbsp;';
      buf += '<input type="submit" value="Create" />';
      buf += '</form>';
      buf += '</td></tr>';
      buf += '</table>';
      
      $("#group_info").html(buf);
    }
  });
});