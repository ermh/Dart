/*
 * Copyright (c) 2011, the Dart project authors.
 * 
 * Licensed under the Eclipse Public License v1.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 * 
 * http://www.eclipse.org/legal/epl-v10.html
 * 
 * Unless required by applicable law or agreed to in writing, software distributed under the License
 * is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
 * or implied. See the License for the specific language governing permissions and limitations under
 * the License.
 */
package com.google.dart.indexer.index;

import com.google.dart.indexer.IndexerPlugin;
import com.google.dart.indexer.debug.IndexerDebugOptions;
import com.google.dart.indexer.exceptions.IndexRequestFailed;
import com.google.dart.indexer.exceptions.IndexRequestFailedUnchecked;
import com.google.dart.indexer.index.configuration.IndexConfigurationInstance;
import com.google.dart.indexer.index.configuration.Processor;
import com.google.dart.indexer.index.entries.DependentEntity;
import com.google.dart.indexer.index.entries.DependentFileInfo;
import com.google.dart.indexer.index.entries.DependentLocation;
import com.google.dart.indexer.index.entries.FileInfo;
import com.google.dart.indexer.index.entries.LocationInfo;
import com.google.dart.indexer.index.layers.Layer;
import com.google.dart.indexer.index.updating.FileInfoUpdater;
import com.google.dart.indexer.index.updating.FileInfoUpdaterImpl;
import com.google.dart.indexer.locations.Location;
import com.google.dart.indexer.storage.FileTransaction;
import com.google.dart.indexer.storage.StorageTransaction;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.runtime.Platform;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;

/**
 * Executes an operation consisting of reindexing some files and removing some files, so that the
 * index is reliably updated as a result of this operation.
 */
public class IndexTransaction {
  private final static boolean TRACE_INDEXED_FILES = Boolean.valueOf(
      Platform.getDebugOption(IndexerDebugOptions.INDEXED_FILES)).booleanValue();

  private final IndexConfigurationInstance configuration;

  private ArrayList<IFile> filesWithErrors = new ArrayList<IFile>();
  private ArrayList<Throwable> errors = new ArrayList<Throwable>();

  private final StorageTransaction storageTransaction;

  public IndexTransaction(StorageTransaction storageTransaction,
      IndexConfigurationInstance configuration) {
    if (storageTransaction == null) {
      throw new NullPointerException("storageTransaction is null");
    }
    if (configuration == null) {
      throw new NullPointerException("configuration is null");
    }
    this.storageTransaction = storageTransaction;
    this.configuration = configuration;
  }

  public void addErrorFile(IFile file, Throwable error) {
    filesWithErrors.add(file);
    errors.add(error);
  }

  public void close() throws IndexRequestFailed {
    for (Processor processor : configuration.getKnownProcessors()) {
      try {
        processor.transactionEnded();
      } catch (Exception exception) {
        // Ignore problems from individual processors, other than to log them.
        IndexerPlugin.getLogger().logError(exception,
            "Processor failed while transaction was ending: " + processor.getClass().getName());
      }
    }
    storageTransaction.setErrorFilesList(filesWithErrors);
    storageTransaction.commit();
  }

  public Throwable[] getErrors() {
    return errors.toArray(new Throwable[errors.size()]);
  }

  public IFile[] getFilesWithErrors() {
    return filesWithErrors.toArray(new IFile[filesWithErrors.size()]);
  }

  public void indexFile(IFile file) throws IndexRequestFailed {
    try {
      Processor[] processors = configuration.findProcessors(file);
      if (processors.length == 0) {
        return;
      }
      if (TRACE_INDEXED_FILES) {
        IndexerPlugin.getLogger().trace("Indexing " + file.getFullPath());
      }
      FileTransaction fileTransaction = storageTransaction.createFileTransaction(file);
      removeInformationThatWillBeReconstructed(file, fileTransaction.getOriginalFileInfo());
      FileInfoUpdater updater = new FileInfoUpdaterImpl(fileTransaction, file);
      for (int i = 0; i < processors.length; i++) {
        processors[i].processFile(file, updater);
      }
      fileTransaction.commit();
    } catch (IndexRequestFailedUnchecked e) {
      throw e.unwrap();
    }
  }

  public IFile[] removeFile(IFile file) {
    FileInfo info = storageTransaction.removeFileInfo(file);
    if (info == null) {
      return new IFile[0];
    }
    removeInformationThatWillBeReconstructed(file, info);
    Collection<Location> sourceLocations = info.getSourceLocations();
    Set<IFile> affectedFiles = calculateFilesAffectedByRemovalOf(sourceLocations);
    for (Iterator<Location> iterator = sourceLocations.iterator(); iterator.hasNext();) {
      storageTransaction.removeLocationInfo(iterator.next());
    }
    return affectedFiles.toArray(new IFile[affectedFiles.size()]);
  }

  private Set<IFile> calculateFilesAffectedByRemovalOf(Collection<Location> sourceLocations) {
    Set<IFile> affectedFiles = new HashSet<IFile>();
    for (Iterator<Location> iterator = sourceLocations.iterator(); iterator.hasNext();) {
      calculateFilesAffectedByRemovalOf(iterator.next(), affectedFiles);
    }
    return affectedFiles;
  }

  private void calculateFilesAffectedByRemovalOf(Layer layer, Location location, Set<IFile> result) {
    LocationInfo info = storageTransaction.readLocationInfo(layer, location);
    if (info == null) {
      return;
    }
    Location[] affectedLocations = info.getLocationsAffectedByRemovalOfSelf();
    for (int i = 0; i < affectedLocations.length; i++) {
      Location affectedLocation = affectedLocations[i];
      IFile containingFile = affectedLocation.getContainingFile();
      if (containingFile != null) {
        result.add(containingFile);
      }
    }
  }

  private void calculateFilesAffectedByRemovalOf(Location location, Set<IFile> result) {
    Layer[] layers = configuration.getLayers();
    for (int i = 0; i < layers.length; i++) {
      calculateFilesAffectedByRemovalOf(layers[i], location, result);
    }
  }

  private void removeInformationThatWillBeReconstructed(IFile staleFile, FileInfo fileInfo) {
    if (fileInfo == null) {
      return;
    }
    HashSet<Location> staleLocations = new HashSet<Location>(fileInfo.getSourceLocations());
    for (Iterator<DependentEntity> iterator = fileInfo.getInternalDependencies().iterator(); iterator.hasNext();) {
      removeInformationThatWillBeReconstructed(staleFile, staleLocations, iterator.next());
    }
    for (Iterator<DependentEntity> iterator = fileInfo.getExternalDependencies().iterator(); iterator.hasNext();) {
      removeInformationThatWillBeReconstructed(staleFile, staleLocations, iterator.next());
    }
    fileInfo.clearInternalDependencies();
  }

  private void removeInformationThatWillBeReconstructed(IFile staleFile,
      HashSet<Location> staleLocations, DependentEntity dependency) {
    if (dependency instanceof DependentFileInfo) {
      DependentFileInfo dfi = (DependentFileInfo) dependency;
      storageTransaction.removeStaleDependencies(dfi.getFile(), staleFile, staleLocations);
    } else if (dependency instanceof DependentLocation) {
      DependentLocation dl = (DependentLocation) dependency;
      removeStaleInformationAboutLocations(dl.getDependentLayer(), staleLocations,
          dl.getDependentLocation());
    }
  }

  private void removeStaleInformationAboutLocations(Layer layer,
      HashSet<Location> staleSourceLocations, Location destination) {
    storageTransaction.removeStaleLocationsFromDestination(layer, destination, staleSourceLocations);
  }
}
