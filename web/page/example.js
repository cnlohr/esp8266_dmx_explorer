//Copyright (C) 2016 <>< Charles Lohr, see LICENSE file for more info.
//
//This particular file may be licensed under the MIT/x11, New BSD or ColorChord Licenses.

var NumChannels = 512|0;
var ChannelData = [];

function UpdateNumChannels()
{
	NumChannels = Number( $('#IdNumChannels').val() );
	var HTMLChannels = "<TABLE BORDER=1><TR><TH></TH>";
	var i = 0|0;
	for( i = 0; i < 16; i++ )
	{
		HTMLChannels += "<TH>" + tohex8(i) + "</TH>";
	}
	for( i = 0; i < NumChannels; i++ )
	{
		if( ChannelData.length <= i ) ChannelData.push( 20|0 );
		if( (i % 16) == 0 ) HTMLChannels += "</TR><TR><TD>" + tohex8(i) + "</TD>";
		HTMLChannels += "<TD><INPUT TYPE=number SIZE=3 MIN=0 MAX=255 ID=ChannelData" + i + " VALUE=" + ChannelData[i] + " onchange=UpdateChannels(" +i+ ");></TD>";
	}	
	HTMLChannels += "</TR></TABLE>";
	$('#DmxDspl').html( HTMLChannels );

	$('#BtnSetNumChannels').val('Set Num Channels');
	IssueSystemMessage( "Set number of channels." );
}

function ConfigureNumChannels( chan )
{
	NumChannels = chan|0;
	var St = "CX"+String.fromCharCode(NumChannels%256) + String.fromCharCode(NumChannels>>8);
	QueueOperation( St, UpdateNumChannels ); // Send info request to ESP
}

function UpdateSent()
{
}

function UpdateChannels( chan )
{
	if( chan >= 0 )
	{
		ChannelData[chan] = $("#ChannelData"+chan).val() | 0;
	}
	//Let's not allow partial updates.
	chan = -1;

	var offset = ((chan>=0)?chan:0)|0;
	var nChToSend = (chan>=0)?1:NumChannels;

	sendarray = new Uint8Array( nChToSend + 4 );
	sendarray[0] = 'C'.charCodeAt(0);
	sendarray[1] = 'D'.charCodeAt(0);
	sendarray[2] = offset % 256;
	sendarray[3] = offset >> 8;

	var i = 0|0;
	console.log( " " + offset + " " + nChToSend + " " + ChannelData[0] + " " + ChannelData[1] );
	for( i = 0; i < nChToSend; i++ )
	{
		sendarray[i+4] = ChannelData[i] | 0;
	}
	QueueOperation( sendarray, UpdateSent ); // Send info request to ESP
}

function UpdateChannelsTimer()
{
	setTimeout( UpdateChannelsTimer, 100 );
	UpdateChannels(-1);
}

UpdateChannelsTimer();



function initDMX512() {
var menItm = `
	<tr><td width=1><input type=submit onclick="ShowHideEvent( 'DMX512' );" value="DMX-512"></td><td>
	<div id=DMX512 class="collapsible">
	<input type=text id=IdNumChannels><input type=button id=BtnSetNumChannels value="Set Num Channels"><br>
	<p id=DmxDspl>&nbsp;</p>
	</div>
	</td></tr>
`;
	$('#MainMenu > tbody:last').after( menItm );
	$('#IdNumChannels').val( NumChannels );

	ConfigureNumChannels( NumChannels );

	$('#BtnSetNumChannels').click( function(e) {
		$('#BtnSetNumChannels').val('Setting Channels...');
		ConfigureNumChannels( $('#IdNumChannels').val() );
	});
}

window.addEventListener("load", initDMX512, false);


