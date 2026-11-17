/* Original live bridge for SmileMorph Precomp. Select a precomp layer outside.
   An independent parent + source copy is created; the original is preserved. */
function SmileMorphSetup(target, firstTime, lastTime) {
    if (!target || !(target.source instanceof CompItem)) throw Error('Select one precomp layer in its parent composition.');
    if (!target.property('ADBE Effect Parade').canAddProperty('SmileMorph Precomp Native'))
        throw Error('Install SmileMorphPrecomp.aex and restart After Effects first.');
    var parent=target.containingComp, originalSource=target.source, guide=null, i,j;
    for(i=1;i<=originalSource.numLayers;i++) {
        var candidate=originalSource.layer(i), masks=candidate.property('ADBE Mask Parade'), count=0;
        if(masks)for(j=1;j<=masks.numProperties;j++)if(!masks.property(j).property('ADBE Mask Shape').value.closed)count++;
        if(count && (!guide || count>guide.count))guide={index:i,count:count};
    }
    if(!guide)throw Error('No open guide masks found inside the precomp.');
    if(target.timeRemapEnabled || target.stretch<=0)throw Error('Set up on a forward precomp without Time Remapping. Reverse or retime the resulting composition afterwards.');
    if(target.property('ADBE Mask Parade').numProperties)throw Error('Use an unmasked outer precomp layer. The editable guides belong inside the source comp.');
    // Reuse picture endpoints when this is a RE:Flex reference, without assuming
    // a frame rate, duration, number of frames, or particular parameter index.
    var fx=target.property('ADBE Effect Parade'), endpoints=[];
    for(i=1;i<=fx.numProperties;i++)if(fx.property(i).matchName==='ReelSmart Morph2') {
        for(j=1;j<=fx.property(i).numProperties;j++) {
            var p=fx.property(i).property(j);
            if(p.propertyType===PropertyType.PROPERTY && p.name.indexOf('Picture Key')>=0)
                for(var k=1;k<=p.numKeys;k++)if(p.keyValue(k)>0)endpoints.push(p.keyTime(k));
        }
    }
    if(firstTime===undefined)firstTime=endpoints.length>=2?endpoints[0]:target.inPoint;
    if(lastTime===undefined)lastTime=endpoints.length>=2?endpoints[endpoints.length-1]:Math.min(target.outPoint-parent.frameDuration,parent.workAreaStart+parent.workAreaDuration);
    if(!(lastTime>firstTime))throw Error('Choose two different picture endpoint times.');
    app.beginUndoGroup('Create independent SmileMorph Precomp test');
    try {
        var copy=parent.duplicate(), src=originalSource.duplicate();
        copy.name='SmileMorph test - '+parent.name;
        src.name='SM source '+src.id;
        var layer=copy.layer(target.index);layer.replaceSource(src,false);
        // Remove the reference effect only from the duplicate being tested.
        var effects=layer.property('ADBE Effect Parade');
        for(i=effects.numProperties;i>=1;i--)if(effects.property(i).matchName==='ReelSmart Morph2')effects.property(i).remove();
        var bridge=layer.property('ADBE Mask Parade');
        if(bridge.numProperties)throw Error('Outer layer already has masks. Use an unmasked precomp layer. Undo removes this test copy.');
        var effect=effects.addProperty('SmileMorph Precomp Native');effect.name='SmileMorph Precomp';
        var progress=effect.property(1);progress.setValueAtTime(firstTime,0);progress.setValueAtTime(lastTime,100);
        for(i=1;i<=2;i++)progress.setInterpolationTypeAtKey(i,KeyframeInterpolationType.LINEAR,KeyframeInterpolationType.LINEAR);
        effect.property(2).expression='var p=thisProperty.propertyGroup(1)(1); thisLayer.sourceTime(p.key(1).time);';
        effect.property(3).expression='var p=thisProperty.propertyGroup(1)(1); thisLayer.sourceTime(p.key(p.numKeys).time);';
        effect.property(4).setValue(192);
        var inner=src.layer(guide.index), innerMasks=inner.property('ADBE Mask Parade');
        for(i=1;i<=innerMasks.numProperties;i++) {
            var original=innerMasks.property(i), path=original.property('ADBE Mask Shape');
            if(path.value.closed)continue;
            for(var role=0;role<3;role++){
            var m=bridge.addProperty('ADBE Mask Atom');m.name=['SM A ','SM B ','SM C '][role]+i;m.maskMode=MaskMode.NONE;
            // Sample directly from the inner mask, including its animated Bezier
            // tangents and layer transforms. No baked coordinates or fixed timing.
            m.property('ADBE Mask Shape').expression=
                'var c=comp("'+src.name+'");\n'+
                'var l=c.layer('+guide.index+');\n'+
                'var pr=effect("SmileMorph Precomp")(1);\n'+
                'var t=thisLayer.sourceTime('+['pr.key(1).time','pr.key(pr.numKeys).time','time'][role]+');\n'+
                'var p=l.mask('+i+').maskPath;\n'+
                'var v=p.points(t), a=p.inTangents(t), b=p.outTangents(t);\n'+
                'var V=[],A=[],B=[];\n'+
                'for(var n=0;n<v.length;n++){\n'+
                ' var q=l.toComp(v[n],t);V.push([q[0],q[1]]);\n'+
                ' var u=l.toComp(v[n]+a[n],t)-q;A.push([u[0],u[1]]);\n'+
                ' var w=l.toComp(v[n]+b[n],t)-q;B.push([w[0],w[1]]);}\n'+
                'createPath(V,A,B,false);';
            }
        }
        for(i=1;i<=copy.numLayers;i++)if(copy.layer(i).enabled)copy.layer(i).solo=false;
        layer.enabled=true;layer.solo=true;copy.time=(firstTime+lastTime)/2;
        copy.openInViewer();layer.selected=true;
        return {parent:copy,source:src,layer:layer,effect:effect};
    } finally {app.endUndoGroup();}
}
if(!$.global.SMILEMORPH_LIBRARY_ONLY) {
    try {
        var c=app.project.activeItem;
        if(!(c instanceof CompItem)||c.selectedLayers.length!==1)throw Error('Select exactly one precomp layer in its parent composition.');
        SmileMorphSetup(c.selectedLayers[0]);
    }catch(e){alert('SmileMorph Precomp: '+e.toString());}
}
