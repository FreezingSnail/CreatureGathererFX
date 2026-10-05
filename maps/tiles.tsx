<?xml version="1.0" encoding="UTF-8"?>
<tileset version="1.10" tiledversion="1.11.2" name="tiles" tilewidth="16" tileheight="16" tilecount="768" columns="16">
 <image source="../images/tiles.png" width="256" height="768"/>
 <!-- Explicit collision defaults for every non-empty tile used by world_map.json.
      The interior floor at global GID 275 is the only confirmed walkable tile. -->
 <tile id="0"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="6"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="7"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="14"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="22"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="23"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="29"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="31"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="37"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="46"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="62"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="257"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="259"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="260"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="261"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="266"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="273"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="274"><properties><property name="walkable" type="bool" value="true"/></properties></tile>
 <tile id="275"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="289"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="290"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="291"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="296"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="297"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <!-- Authored device fixtures live on the sealed south border. The neighboring
      empty cells keep this region outside normal overworld movement. -->
 <tile id="526"><properties><property name="water" type="bool" value="true"/></properties></tile>
 <tile id="527"><properties><property name="walkable" type="bool" value="true"/><property name="water" type="bool" value="true"/></properties></tile>
 <tile id="528"><properties><property name="walkable" type="bool" value="true"/><property name="encounter" type="bool" value="true"/></properties></tile>
</tileset>
